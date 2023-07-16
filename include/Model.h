#ifndef MODEL_H
#define MODEL_H
// 可参考https://blog.csdn.net/qq_41041725/article/details/121959175对PBR做改进
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
#include <ranges>
#include <algorithm>
#include <memory>
namespace ModelLoader
{

    class Model
    {
    public:
        // model data
        std::vector<std::shared_ptr<Mesh>> meshes;
        std::vector<std::shared_ptr<Renderer::Texture>> textures_loaded; // stores all the textures loaded so far, optimization to make sure textures aren't loaded more than once.
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
        ~Model()
        {
            for (auto &mesh : meshes)
            {
                mesh.reset();
            }
            for (auto &texture : textures_loaded)
            {
                texture.reset();
            }
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

        std::shared_ptr<Mesh> processMesh(aiMesh *mesh, const aiScene *scene, bool usePBR)
        {
            // data to fill
            std::vector<Vertex> vertices;
            std::vector<unsigned int> indices;
            std::vector<std::shared_ptr<Renderer::Texture>> textures;

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
                std::vector<std::shared_ptr<Renderer::Texture>> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "material.texture_diffuse");
                textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
                // 2. specular maps
                std::vector<std::shared_ptr<Renderer::Texture>> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "material.texture_specular");
                textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
                // 3. normal maps
                std::vector<std::shared_ptr<Renderer::Texture>> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "material.texture_normal");
                textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
                // 4. height maps
                std::vector<std::shared_ptr<Renderer::Texture>> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "material.texture_height");
                textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

                // return a mesh object created from the extracted mesh data
                return std::make_shared<Mesh>(vertices, indices, textures, usePBR);
            }
            else
            {
                PBRMaterial pbrMat;
                // 这里有个麻烦就是如果是gltf材质的话，金属度和粗糙度贴图是合并在一张贴图的，所以实际上读取的时候是读取的金属度贴图，然后把金属度贴图的b通道作为金属度，g通道作为粗糙度(所以pbr.frag里面的metallic和roughness是用的同一张贴图，但是分别用的不同通道)，算是特殊处理
                //  1. albedo
                if (material->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "material.albedoMap", true);
                    textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
                    pbrMat.useAlbedoMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useAlbedoMap = GL_FALSE;
                    aiColor3D color;
                    material->Get(AI_MATKEY_BASE_COLOR, color);
                    pbrMat.albedo = glm::vec3(color.r, color.g, color.b);
                }

                // 2. normal maps
                if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "material.normalMap");
                    textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
                    pbrMat.useNormalMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useNormalMap = GL_FALSE;
                }
                // 3. metallic maps
                if (material->GetTextureCount(aiTextureType_METALNESS) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "material.metallicMap");
                    textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());
                    pbrMat.useMetallicMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useMetallicMap = GL_FALSE;
                    material->Get(AI_MATKEY_METALLIC_FACTOR, pbrMat.metallic);
                }
                // 4. roughness maps
                if (material->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "material.roughnessMap");
                    textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());
                    pbrMat.useRoughnessMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useRoughnessMap = GL_FALSE;
                    material->Get(AI_MATKEY_ROUGHNESS_FACTOR, pbrMat.roughness);
                }
                // 5. ao maps
                if (material->GetTextureCount(aiTextureType::aiTextureType_AMBIENT_OCCLUSION) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> aoMaps = loadMaterialTextures(material, aiTextureType::aiTextureType_AMBIENT_OCCLUSION, "material.aoMap");
                    textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
                    pbrMat.useAOMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useAOMap = GL_FALSE;
                }
                // 6. emissive maps
                if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
                {
                    std::vector<std::shared_ptr<Renderer::Texture>> emissiveMaps = loadMaterialTextures(material, aiTextureType_EMISSIVE, "material.emissiveMap");
                    textures.insert(textures.end(), emissiveMaps.begin(), emissiveMaps.end());
                    pbrMat.useEmissiveMap = GL_TRUE;
                }
                else
                {
                    pbrMat.useEmissiveMap = GL_FALSE;
                }

                // return a mesh object created from the extracted mesh data
                return std::make_shared<Mesh>(vertices, indices, textures, usePBR, pbrMat);
            }
        }

        // checks all material textures of a given type and loads the textures if they're not loaded yet.
        // the required info is returned as a Texture struct.
        std::vector<std::shared_ptr<Renderer::Texture>> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, bool gammaCorrection = false)
        {
            std::vector<std::shared_ptr<Renderer::Texture>> textures;

            for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
            {
                aiString str;
                mat->GetTexture(type, i, &str);

                bool skip = false;
                std::string filepath = directory + '/' + str.C_Str();
                // 先遍历textures_loaded，看看是否已经加载过了
                for (auto &texture : textures_loaded)
                {
                    // 如果已经加载过了，那么直接把这个纹理加入到textures中
                    if (std::strcmp(texture->path.c_str(), filepath.c_str()) == 0 && texture->type == typeName)
                    {
                        textures.push_back(texture);
                        skip = true;
                        break;
                    }
                }
                // 如果纹理还没有被加载过，那么加载它
                if (!skip)
                {
                    auto &tex = textures.emplace_back(std::make_shared<Renderer::Texture>(str.C_Str(), this->directory, gammaCorrection));
                    tex->SetTextureType(typeName);
                    textures_loaded.push_back(tex);
                }
            }
            return textures;
        }
    };
}
#endif