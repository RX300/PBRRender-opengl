#pragma once
#include <string>
#include <vector>
#include "Model.h"
#include "Skybox.h"
namespace Renderer
{
    class Scene
    {
    public:
        Scene() = default;
        Scene(const std::string &sceneName) : m_sceneName(sceneName)
        {
            std::cout << "Loading scene: " << m_sceneName << std::endl;
            m_skybox = std::make_shared<Skybox>();
        }
        ~Scene()
        {
        }
        // 获取当前类的指针
        Scene *GetScenePtr()
        {
            return this;
        }
        void AddModel(const std::shared_ptr<ModelLoader::Model> &model)
        {
            m_models.push_back(model);
        }
        void LoadSkybox(const char *hdrPath, const std::size_t resolution, GLFWwindow *window)
        {
            m_skybox->Load(hdrPath, resolution, window);
        }
        void SetSkybox(glm::mat4 projection)
        {
            m_skybox->setProjection(projection);
        }
        void Bake(GLFWwindow *window)
        {
            m_skybox->bakeIBL(window);
        }
        void Update(Shader &shader, Camera &camera)
        {
        }
        auto GetModels() { return m_models; }
        auto GetSkybox() { return m_skybox; }
        void renderSphere();
        glm::vec3 lightPositions[4] = {
            glm::vec3(-10.0f, 10.0f, 10.0f),
            glm::vec3(10.0f, 10.0f, 10.0f),
            glm::vec3(-10.0f, -10.0f, 10.0f),
            glm::vec3(10.0f, -10.0f, 10.0f),
        };
        glm::vec3 lightColors[4] = {
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f),
            glm::vec3(300.0f, 300.0f, 300.0f)};

    private:
        std::shared_ptr<Skybox> m_skybox;
        std::string m_sceneName;
        std::vector<std::shared_ptr<ModelLoader::Model>> m_models;
    };

}