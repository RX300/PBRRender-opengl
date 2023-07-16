#pragma once
#include "RenderQueue.h"
inline void renderSphere();
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

    auto resolution = window->GetFramebufferDims();
    glm::mat4 projection = glm::perspective(glm::radians(cam->Zoom), (float)resolution.first / (float)resolution.second, 0.1f, 100.0f);
    shader->setMat4("projection", projection);
    shader->unuse();
    scene->SetSkybox(projection);
    scene->Bake(window->GetWindow());
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
        glm::vec3 newPos = scene->lightPositions[i] + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
        newPos = scene->lightPositions[i];
        shader->setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
        shader->setVec3("lightColors[" + std::to_string(i) + "]", scene->lightColors[i]);

        model = glm::mat4(1.0f);
        model = glm::translate(model, newPos);
        model = glm::scale(model, glm::vec3(0.5f));
        shader->setMat4("model", model);
        shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        renderSphere();
    }
    shader->unuse();
    // render skybox (render as last to prevent overdraw)
    skybox->DrawSkybox(view);
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