#pragma once

#include <glad/glad.h> // holds all OpenGL type declarations

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
        float metallic = 0.0f;
        float roughness = 0.0f;

        // 是否用各项属性的贴图
        bool useAlbedoMap = false;
        bool useNormalMap = false;
        bool useMetallicMap = false;
        bool useRoughnessMap = false;
        bool useAOMap = false;
    };

    class Mesh
    {
    public:
        // mesh Data
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Renderer::Texture> textures;
        PBRMaterial pbrmat;
        unsigned int VAO;
        bool usePBR;
        // constructor
        Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Renderer::Texture> textures, bool PBR, PBRMaterial pbr = PBRMaterial()) : usePBR(PBR)
        {
            this->vertices = vertices;
            this->indices = indices;
            this->textures = std::move(textures);
            this->pbrmat = pbr;
            // now that we have all the required data, set the vertex buffers and its attribute pointers.
            setupMesh();
        }
        ~Mesh()
        {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
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
                    std::string name = textures[i].type;
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
                    glBindTexture(GL_TEXTURE_2D, textures[i].id);
                }
            }
            else
            {
                // 这里先不考虑用对同一参数用多张纹理贴图的情况
                shader.setBool("material.useAlbedoMap", pbrmat.useAlbedoMap);
                shader.setBool("material.useNormalMap", pbrmat.useNormalMap);
                shader.setBool("material.useMetallicMap", pbrmat.useMetallicMap);
                shader.setBool("material.useRoughnessMap", pbrmat.useRoughnessMap);
                shader.setBool("material.useAOMap", pbrmat.useAOMap);
                shader.setVec3("material.albedo", pbrmat.albedo);
                shader.setFloat("material.metallic", pbrmat.metallic);
                shader.setFloat("material.roughness", pbrmat.roughness);

                // bind appropriate textures
                for (unsigned int i = 0; i < textures.size(); i++)
                {
                    // 获取纹理序号（diffuse_textureN 中的 N）
                    std::string number;
                    std::string name = textures[i].type;
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
                    glBindTexture(GL_TEXTURE_2D, textures[i].id);
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
