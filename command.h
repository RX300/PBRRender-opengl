#pragma once
#include "RenderQueue.h"
#include "GBuffer.h"
inline void renderSphere();
inline void renderQuad();
inline void renderCube();
inline void pbrInitFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    shader->use();
    shader->setInt("irradianceMap", 0);
    shader->setInt("prefilterMap", 1);
    shader->setInt("brdfLUT", 2);
    shader->setInt("material.albedoMap", 3);
    shader->setInt("material.normalMap", 4);
    shader->setInt("material.metallicMap", 5);
    shader->setInt("material.roughnessMap", 6);
    shader->setInt("material.aoMap", 7);
    shader->setVec3("albedo", 0.5f, 0.0f, 0.0f);
    shader->setFloat("ao", 1.0f);

    auto resolution = window->GetFramebufferDims();
    glm::mat4 projection = glm::perspective(glm::radians(cam->Zoom), (float)resolution.first / (float)resolution.second, 0.1f, 100.0f);
    shader->setMat4("projection", projection);
    shader->unuse();
    scene->SetSkybox(projection);
    scene->Bake(window->GetWindow());
    shader->unuse();
}
inline void pbrRenderFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    shader->use();
    glm::mat4 model = glm::mat4(1.0f);
    glm::mat4 view = cam->GetViewMatrix();
    shader->setMat4("view", view);
    shader->setVec3("camPos", cam->Position);
    auto skybox = scene->GetSkybox();
    // bind pre-computed IBL data
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->GetIrradianceMap());
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->GetPrefilterMap());
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, skybox->GetBRDFLUTMap());

    model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.25f));
    shader->setMat4("model", model);
    shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    auto modelsptr = scene->GetModels();
    for (auto &modelptr : modelsptr)
    {
        modelptr->Draw(*shader);
    }

    // render light source (simply re-render sphere at light positions)
    // this looks a bit off as we use the same shader, but it'll make their positions obvious and
    // keeps the codeprint small.
    for (unsigned int i = 0; i < sizeof(scene->lightPositions) / sizeof(scene->lightPositions[0]); ++i)
    {
        glm::vec3 newPos = scene->lightPositions[i];
        shader->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
        shader->setVec3("lightColors[" + std::to_string(i) + "]", scene->lightColors[i]);

        model = glm::mat4(1.0f);
        model = glm::translate(model, newPos);
        model = glm::scale(model, glm::vec3(0.5f));
        shader->setMat4("model", model);
        shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        renderSphere();
    }

    // render skybox (render as last to prevent overdraw)
    skybox->DrawSkybox(view);
}

Renderer::GBuffer gBuffer{};
void inline deferredInitFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    auto resolution = window->GetFramebufferDims();
    gBuffer.Load(resolution.first, resolution.second);
    // 几何pass
    shader->use();
    glm::mat4 projection = glm::perspective(glm::radians(cam->Zoom), (float)resolution.first / (float)resolution.second, 0.1f, 1000.0f);
    shader->setMat4("projection", projection);
    shader->unuse();
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
}

void inline lightBoxInitFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    auto resolution = window->GetFramebufferDims();
    shader->use();
    glm::mat4 projection = glm::perspective(glm::radians(cam->Zoom), (float)resolution.first / (float)resolution.second, 0.1f, 1000.0f);
    shader->setMat4("projection", projection);
    shader->unuse();
}

void inline deferredRenderGeometryFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    // 几何pass
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer.m_gBuffer);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    shader->use();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(1.0f));
    shader->setMat4("model", model);
    glm::mat4 view = cam->GetViewMatrix();
    shader->setMat4("view", view);
    shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
    auto modelsptr = scene->GetModels();
    for (auto &modelptr : modelsptr)
    {
        modelptr->Draw(*shader);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    shader->unuse();
}

void inline deferredRenderShaderFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    // 着色pass
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilMask(0x00);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // 这里需要传一次模板缓存，在画完后因为画布的深度会被设置成画布本身的深度，所以后面还需要传一次深度缓存(或者先关闭深度测试，后面在开启)
    glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer.m_gBuffer);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // write to default framebuffer
    glBlitFramebuffer(0, 0, gBuffer.m_width, gBuffer.m_height, 0, 0, gBuffer.m_width, gBuffer.m_height, GL_STENCIL_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    shader->use();
    shader->setVec3("camPos", cam->Position);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gBuffer.m_gPositionRoughness);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gBuffer.m_gNormalAO);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gBuffer.m_gAlbedoMetallic);
    // render light source (simply re-render sphere at light positions)
    // this looks a bit off as we use the same shader, but it'll make their positions obvious and
    // keeps the codeprint small.
    for (unsigned int i = 0; i < sizeof(scene->lightPositions) / sizeof(scene->lightPositions[0]); ++i)
    {
        glm::vec3 newPos = scene->lightPositions[i];
        shader->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
        shader->setVec3("lightColors[" + std::to_string(i) + "]", scene->lightColors[i]);

        // model = glm::mat4(1.0f);
        // model = glm::translate(model, newPos);
        // model = glm::scale(model, glm::vec3(0.5f));
        // shader->setMat4("model", model);
        // shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        // renderSphere();
    }
    renderQuad();
    shader->unuse();
    glEnable(GL_DEPTH_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilMask(0xFF);
}

inline void lightBoxShaderFunc(Renderer::Shader *shader, Camera *cam, Renderer::WindowSystem *window, Renderer::Scene *scene)
{
    // 着色pass
    shader->use();
    shader->setMat4("view", cam->GetViewMatrix());
    glm::mat4 model;
    for (unsigned int i = 0; i < sizeof(scene->lightPositions) / sizeof(scene->lightPositions[0]); ++i)
    {
        model = glm::mat4(1.0f);
        model = glm::translate(model, scene->lightPositions[i]);
        model = glm::scale(model, glm::vec3(0.125f));
        shader->setMat4("model", model);
        shader->setVec3("lightColor", scene->lightColors[i]);
        renderCube();
    }
}

// renders (and builds at first invocation) a sphere
// -------------------------------------------------
inline unsigned int sphereVAO = 0;
inline unsigned int indexCount;
inline void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

// RenderQuad() Renders a 1x1 quad in NDC, best used for framebuffer color targets
// and post-processing effects.
GLuint quadVAO = 0;
GLuint quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        GLfloat quadVertices[] = {
            // Positions        // Texture Coords
            -1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            -1.0f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            -1.0f,
            0.0f,
            1.0f,
            0.0f,
        };
        // Setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// RenderCube() Renders a 1x1 3D cube in NDC.
GLuint cubeVAO = 0;
GLuint cubeVBO = 0;
void renderCube()
{
    // Initialize (if necessary)
    if (cubeVAO == 0)
    {
        GLfloat vertices[] = {
            // Back face
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
            // Front face
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
            // Left face
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
            // Right face
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-right
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-right
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-left
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-left
            // Bottom face
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
            // Top face
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // Fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // Link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid *)(6 * sizeof(GLfloat)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // Render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}