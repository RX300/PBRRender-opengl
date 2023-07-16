#pragma once

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"
#include "Texture.h"

#include <string>
#include <vector>

namespace ModelLoader
{
#define MAX_BONE_INFLUENCE 4

    struct Vertex
    {
        // position
        glm::vec3 Position;
        // normal
        glm::vec3 Normal;
        // texCoords
        glm::vec2 TexCoords;
        // tangent
        glm::vec3 Tangent;
        // bitangent
        glm::vec3 Bitangent;
        // bone indexes which will influence this vertex
        int m_BoneIDs[MAX_BONE_INFLUENCE];
        // weights from each bone
        float m_Weights[MAX_BONE_INFLUENCE];
    };

    struct PBRMaterial
    {
        // material properties
        glm::vec3 albedo = glm::vec3(0.0f);
        GLfloat metallic = 1.0f;
        GLfloat roughness = 0.0f;

        // 是否用各项属性的贴图
        GLboolean useAlbedoMap = false;
        GLboolean useNormalMap = false;
        GLboolean useMetallicMap = false;
        GLboolean useRoughnessMap = false;
        GLboolean useAOMap = false;
        GLboolean useEmissiveMap = false;
    };

    class Mesh
    {
    public:
        // mesh Data
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<std::shared_ptr<Renderer::Texture>> textures;
        PBRMaterial pbrmat;
        unsigned int VAO;
        bool usePBR;
        /*  UBO  */
        GLuint ubo;
        GLuint bindingPoint = 0; // 和pbr.fs中的layout(std140, binding = 0)对应
        GLfloat *ubo_ptr;
        int bool3;
        // constructor
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<std::shared_ptr<Renderer::Texture>> textures, bool PBR, PBRMaterial pbr = PBRMaterial()) : usePBR(PBR)
        {
            this->vertices = vertices;
            this->indices = indices;
            this->textures = textures;
            this->pbrmat = pbr;
            // 生成 UBO
            glGenBuffers(1, &ubo);
            glBindBuffer(GL_UNIFORM_BUFFER, ubo);
            glBufferData(GL_UNIFORM_BUFFER, 44, nullptr, GL_STATIC_DRAW);
            glBindBuffer(GL_UNIFORM_BUFFER, 0);
            glBindBufferRange(GL_UNIFORM_BUFFER, bindingPoint, ubo, 0, 44);
            // now that we have all the required data, set the vertex buffers and its attribute pointers.
            setupMesh();
        }
        ~Mesh()
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
            glDeleteBuffers(1, &ubo);
        }
        // render the mesh
        void Draw(Renderer::Shader &shader)
        {
            if (!usePBR)
            {
                // bind appropriate textures
                unsigned int diffuseNr = 1;
                unsigned int specularNr = 1;
                unsigned int normalNr = 1;
                unsigned int heightNr = 1;
                for (unsigned int i = 0; i < textures.size(); i++)
                {
                    glActiveTexture(GL_TEXTURE0 + i); // 在绑定之前激活相应的纹理单元
                    // 获取纹理序号（diffuse_textureN 中的 N）
                    std::string number;
                    std::string name = textures[i]->type;
                    if (name == "material.texture_diffuse")
                        number = std::to_string(diffuseNr++);
                    else if (name == "material.texture_specular")
                        number = std::to_string(specularNr++); // transfer unsigned int to string
                    else if (name == "material.texture_normal")
                        number = std::to_string(normalNr++); // transfer unsigned int to string
                    else if (name == "material.texture_height")
                        number = std::to_string(heightNr++); // transfer unsigned int to string

                    // 给glsl里的采样器uniform设置纹理单元，采样器的名称相对固定(比如漫反射纹理就是texture_diffuse1,2,3...等)
                    glUniform1i(glGetUniformLocation(shader.ID, (name + number).c_str()), i);
                    // and finally bind the texture
                    glBindTexture(GL_TEXTURE_2D, textures[i]->id);
                }
            }
            else
            {
                // 这里先不考虑用对同一参数用多张纹理贴图的情况
                // bind appropriate textures
                // shader.setVec3("material.albedo", pbrmat.albedo);
                // shader.setFloat("material.metallic", pbrmat.metallic);
                // shader.setFloat("material.roughness", pbrmat.roughness);
                // shader.setBool("material.useAlbedoMap", pbrmat.useAlbedoMap);
                // shader.setBool("material.useNormalMap", pbrmat.useNormalMap);
                // shader.setBool("material.useMetallicMap", pbrmat.useMetallicMap);
                // shader.setBool("material.useRoughnessMap", pbrmat.useRoughnessMap);
                // shader.setBool("material.useAOMap", pbrmat.useAOMap);
                // shader.setBool("material.useEmissiveMap", pbrmat.useEmissiveMap);

                glBindBuffer(GL_UNIFORM_BUFFER, ubo);
                // glBufferSubData(GL_UNIFORM_BUFFER, 0, 4, &temp);
                // 映射ubo
                ubo_ptr = (GLfloat *)glMapBufferRange(GL_UNIFORM_BUFFER, 0, 44, GL_MAP_WRITE_BIT);
                // 检测是否映射成功
                if (ubo_ptr == nullptr)
                {
                    std::cout << "Failed to map uniform buffer object!" << std::endl;
                    exit(-1);
                }
                int boolTemp;
                // memcpy(&ubo_ptr[0], glm::value_ptr(pbrmat.albedo), 12);
                ubo_ptr[0] = pbrmat.albedo.r;
                ubo_ptr[1] = pbrmat.albedo.g;
                ubo_ptr[2] = pbrmat.albedo.b;
                ubo_ptr[3] = pbrmat.metallic;
                ubo_ptr[4] = pbrmat.roughness;
                // std::cout << "pbr attr:" << pbrmat.useAlbedoMap << std::endl;
                // std::cout << "GL_TRUE:" << GL_TRUE << std::endl;
                GLboolean testbool = pbrmat.useAlbedoMap;
                ubo_ptr[5] = testbool;
                ubo_ptr[6] = pbrmat.useNormalMap;
                ubo_ptr[7] = pbrmat.useMetallicMap;
                ubo_ptr[8] = pbrmat.useRoughnessMap;
                ubo_ptr[9] = pbrmat.useAOMap;
                ubo_ptr[10] = pbrmat.useEmissiveMap;
                // 将数据拷贝到映射的缓冲区
                // memcpy(&ubo_ptr[0], glm::value_ptr(pbrmat.albedo), 12);
                // memcpy(&ubo_ptr[16], &pbrmat.metallic, 4);
                // memcpy(&ubo_ptr[20], &pbrmat.roughness, 4);
                // int boolAlbedo = pbrmat.useAlbedoMap;
                // boolTemp = pbrmat.useAlbedoMap;
                // memcpy(&ubo_ptr[24], &boolAlbedo, 4);
                // boolTemp = pbrmat.useNormalMap;
                // memcpy(&ubo_ptr[28], &boolTemp, 4);
                // boolTemp = pbrmat.useMetallicMap;
                // memcpy(&ubo_ptr[32], &boolTemp, 4);
                // boolTemp = pbrmat.useRoughnessMap;
                // memcpy(&ubo_ptr[36], &boolTemp, 4);
                // boolTemp = pbrmat.useAOMap;
                // memcpy(&ubo_ptr[40], &boolTemp, 4);
                // boolTemp = pbrmat.useEmissiveMap;
                // memcpy(&ubo_ptr[44], &boolTemp, 4);
                glUnmapBuffer(GL_UNIFORM_BUFFER);

                glBindBuffer(GL_UNIFORM_BUFFER, 0);

                // bind appropriate textures
                for (unsigned int i = 0; i < textures.size(); i++)
                {
                    // 获取纹理序号（diffuse_textureN 中的 N）
                    std::string number;
                    std::string name = textures[i]->type;
                    if (name == "material.albedoMap")
                    {
                        glActiveTexture(GL_TEXTURE3);
                        glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), 3);
                    }
                    else if (name == "material.normalMap")
                    {
                        glActiveTexture(GL_TEXTURE4);
                        glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), 4);
                    }
                    else if (name == "material.metallicMap")
                    {
                        glActiveTexture(GL_TEXTURE5);
                        glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), 5);
                    }
                    else if (name == "material.roughnessMap")
                    {
                        glActiveTexture(GL_TEXTURE6);
                        glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), 6);
                    }
                    else if (name == "material.aoMap")
                    {
                        glActiveTexture(GL_TEXTURE7);
                        glUniform1i(glGetUniformLocation(shader.ID, (name).c_str()), 7);
                    }

                    // and finally bind the texture
                    glBindTexture(GL_TEXTURE_2D, textures[i]->id);
                }
            }
            // 绘制网格
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(indices.size()), GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);

            // always good practice to set everything back to defaults once configured.
            glActiveTexture(GL_TEXTURE0);
        }

    private:
        // render data
        unsigned int VBO, EBO;

        // initializes all the buffer objects/arrays
        void setupMesh()
        {
            // create buffers/arrays
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glGenBuffers(1, &EBO);

            glBindVertexArray(VAO);
            // load data into vertex buffers
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            // A great thing about structs is that their memory layout is sequential for all its items.
            // The effect is that we can simply pass a pointer to the struct and it translates perfectly to a glm::vec3/2 array which
            // again translates to 3/2 floats which translates to a byte array.
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

            // set the vertex attribute pointers
            // vertex Positions
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
            // vertex normals
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Normal));
            // vertex texture coords
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));
            // vertex tangent
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));
            // vertex bitangent
            glEnableVertexAttribArray(4);
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent));
            // ids
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(Vertex), (void *)offsetof(Vertex, m_BoneIDs));

            // weights
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, m_Weights));
            glBindVertexArray(0);
        }
    };
} // namespace Model
