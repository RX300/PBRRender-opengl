#ifndef MODEL_H
#define MODEL_H

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Mesh.h"
#include "Shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <memory>
namespace ModelLoader
{

    class Model
    {
    public:
        // model data
        std::vector<std::unique_ptr<Mesh>> meshes;
        std::string directory;
        bool gammaCorrection;
        bool usePBR;

        // constructor, expects a filepath to a 3D model.
        Model(std::string const &path, bool gamma = false, bool PBR = false) : gammaCorrection(gamma), usePBR(PBR)
        {
            loadModel(path, PBR);
        }

        // draws the model, and thus all its meshes
        void Draw(Renderer::Shader &shader)
        {
            for (unsigned int i = 0; i < meshes.size(); i++)
                meshes[i]->Draw(shader);
        }

    private:
        // loads a model with supported ASSIMP extensions from file and stores the resulting meshes in the meshes vector.
        void loadModel(std::string const &path, bool usePBR)
        {
            // read file via ASSIMP
            Assimp::Importer importer;
            const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
            // check for errors
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
            {
                std::cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
                return;
            }
            // retrieve the directory path of the filepath
            directory = path.substr(0, path.find_last_of('/'));

            // process ASSIMP's root node recursively
            processNode(scene->mRootNode, scene, usePBR);
        }

        // processes a node in a recursive fashion. Processes each individual mesh located at the node and repeats this process on its children nodes (if any).
        void processNode(aiNode *node, const aiScene *scene, bool usePBR)
        {
            // process each mesh located at the current node
            for (unsigned int i = 0; i < node->mNumMeshes; i++)
            {
                // the node object only contains indices to index the actual objects in the scene.
                // the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
                aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
                meshes.push_back(processMesh(mesh, scene, usePBR));
            }
            // after we've processed all of the meshes (if any) we then recursively process each of the children nodes
            for (unsigned int i = 0; i < node->mNumChildren; i++)
            {
                processNode(node->mChildren[i], scene, usePBR);
            }
        }

        std::unique_ptr<Mesh> processMesh(aiMesh *mesh, const aiScene *scene, bool usePBR)
        {
            // data to fill
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<Renderer::Texture> textures;

            // walk through each of the mesh's vertices
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
            {
                Vertex vertex;
                glm::vec3 vector; // we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this placeholder glm::vec3 first.
                // positions
                vector.x = mesh->mVertices[i].x;
                vector.y = mesh->mVertices[i].y;
                vector.z = mesh->mVertices[i].z;
                vertex.Position = vector;
                // normals
                if (mesh->HasNormals())
                {
                    vector.x = mesh->mNormals[i].x;
                    vector.y = mesh->mNormals[i].y;
                    vector.z = mesh->mNormals[i].z;
                    vertex.Normal = vector;
                }
                // texture coordinates
                if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
                {
                    glm::vec2 vec;
                    // a vertex can contain up to 8 different texture coordinates. We thus make the assumption that we won't
                    // use models where a vertex can have multiple texture coordinates so we always take the first set (0).
                    vec.x = mesh->mTextureCoords[0][i].x;
                    vec.y = mesh->mTextureCoords[0][i].y;
                    vertex.TexCoords = vec;
                    // tangent
                    vector.x = mesh->mTangents[i].x;
                    vector.y = mesh->mTangents[i].y;
                    vector.z = mesh->mTangents[i].z;
                    vertex.Tangent = vector;
                    // bitangent
                    vector.x = mesh->mBitangents[i].x;
                    vector.y = mesh->mBitangents[i].y;
                    vector.z = mesh->mBitangents[i].z;
                    vertex.Bitangent = vector;
                }
                else
                    vertex.TexCoords = glm::vec2(0.0f, 0.0f);

                vertices.push_back(vertex);
            }
            // now wak through each of the mesh's faces (a face is a mesh its triangle) and retrieve the corresponding vertex indices.
            for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            {
                aiFace face = mesh->mFaces[i];
                // retrieve all indices of the face and store them in the indices vector
                for (unsigned int j = 0; j < face.mNumIndices; j++)
                    indices.push_back(face.mIndices[j]);
            }
            // process materials
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            // 不使用PBR
            if (!usePBR)
            {
                // we assume a convention for sampler names in the shaders. Each diffuse texture should be named
                // as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
                // Same applies to other texture as the following list summarizes:
                // diffuse: texture_diffuseN
                // specular: texture_specularN
                // normal: texture_normalN

                // 1. diffuse maps
                std::vector<Renderer::Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "material.texture_diffuse");
                textures.insert(textures.end(), std::make_move_iterator(diffuseMaps.begin()), std::make_move_iterator(diffuseMaps.end()));
                // 2. specular maps
                std::vector<Renderer::Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "material.texture_specular");
                textures.insert(textures.end(), std::make_move_iterator(specularMaps.begin()), std::make_move_iterator(specularMaps.end()));
                // 3. normal maps
                std::vector<Renderer::Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "material.texture_normal");
                textures.insert(textures.end(), std::make_move_iterator(normalMaps.begin()), std::make_move_iterator(normalMaps.end()));
                // 4. height maps
                std::vector<Renderer::Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "material.texture_height");
                textures.insert(textures.end(), std::make_move_iterator(heightMaps.begin()), std::make_move_iterator(heightMaps.end()));

                // return a mesh object created from the extracted mesh data
                return std::make_unique<Mesh>(vertices, indices, std::move(textures), usePBR);
            }
            else
            {
                PBRMaterial pbrMat;
                // 这里有个麻烦就是如果是gltf材质的话，金属度和粗糙度贴图是合并在一张贴图的，所以实际上读取的时候是读取的金属度贴图，然后把金属度贴图的b通道作为金属度，g通道作为粗糙度(所以pbr.frag里面的metallic和roughness是用的同一张贴图，但是分别用的不同通道)，算是特殊处理
                //  1. albedo
                if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
                {
                    std::vector<Renderer::Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "material.albedoMap", true);
                    textures.insert(textures.end(), std::make_move_iterator(albedoMaps.begin()), std::make_move_iterator(albedoMaps.end()));
                    pbrMat.useAlbedoMap = true;
                }
                else
                {
                    pbrMat.useAlbedoMap = false;
                    aiColor3D color;
                    material->Get(AI_MATKEY_BASE_COLOR, color);
                    pbrMat.albedo = glm::vec3(color.r, color.g, color.b);
                }

                // 2. normal maps
                if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
                {
                    std::vector<Renderer::Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "material.normalMap");
                    textures.insert(textures.end(), std::make_move_iterator(normalMaps.begin()), std::make_move_iterator(normalMaps.end()));
                    pbrMat.useNormalMap = true;
                }
                else
                {
                    pbrMat.useNormalMap = false;
                }
                // 3. metallic maps
                if (material->GetTextureCount(aiTextureType_METALNESS) > 0)
                {
                    std::vector<Renderer::Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "material.metallicMap");
                    textures.insert(textures.end(), std::make_move_iterator(metallicMaps.begin()), std::make_move_iterator(metallicMaps.end()));
                    pbrMat.useMetallicMap = true;
                }
                else
                {
                    pbrMat.useMetallicMap = false;
                    material->Get(AI_MATKEY_METALLIC_FACTOR, pbrMat.metallic);
                }
                // 4. roughness maps
                if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
                {
                    std::vector<Renderer::Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "material.roughnessMap");
                    textures.insert(textures.end(), std::make_move_iterator(roughnessMaps.begin()), std::make_move_iterator(roughnessMaps.end()));
                    pbrMat.useRoughnessMap = true;
                }
                else
                {
                    pbrMat.useRoughnessMap = false;
                    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, pbrMat.roughness);
                }
                // 5. ao maps
                if (material->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0)
                {
                    std::vector<Renderer::Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "material.aoMap");
                    textures.insert(textures.end(), std::make_move_iterator(aoMaps.begin()), std::make_move_iterator(aoMaps.end()));
                    pbrMat.useAOMap = true;
                }
                else
                {
                    pbrMat.useAOMap = false;
                }

                // // 这里有个麻烦就是如果是gltf材质的话，金属度和粗糙度贴图是合并在一张贴图的，所以实际上读取的时候是读取的金属度贴图，然后把金属度贴图的b通道作为金属度，g通道作为粗糙度(所以pbr.frag里面的metallic和roughness是用的同一张贴图，但是分别用的不同通道)，算是特殊处理
                // //  1. albedo maps
                // std::vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "material.albedoMap", gammaCorrection);
                // textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
                // // 2. normal maps
                // std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "material.normalMap");
                // textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
                // // 3. metallic maps
                // std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "material.metallicMap");
                // textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
                // // 4. roughness maps
                // std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "material.roughnessMap");
                // textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
                // // 5. ao maps
                // std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "material.aoMap");
                // textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());

                // return a mesh object created from the extracted mesh data
                return std::make_unique<Mesh>(vertices, indices, std::move(textures), usePBR, pbrMat);
            }
        }

        // checks all material textures of a given type and loads the textures if they're not loaded yet.
        // the required info is returned as a Texture struct.
        std::vector<Renderer::Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, bool gammaCorrection = false)
        {
            std::vector<Renderer::Texture> textures;
            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);
                Renderer::Texture tex(str.C_Str(), this->directory, gammaCorrection);
                ;
                tex.SetTextureType(typeName);
                // 这里会隐式调用移动构造函数，所以要主动把tex变成右值
                textures.push_back(std::move(tex));
            }
            return textures;
        }
    };
}
#endif