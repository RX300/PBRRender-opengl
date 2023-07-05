#pragma once
#include "Shader.h"
#include "Camera.h"
#include "Scene.h"
#include "Windowsystem.h"
#include <functional>
#include <tuple>
namespace Renderer
{

    // RemderCommand用于调用实际的渲染函数，渲染函数就是一个function包装的回调函数
    template <typename... Args>
    using RenderFunc = std::function<void(Shader *shader, Camera *cam, WindowSystem *window, Scene *scene, Args &&...args)>;

    template <typename... Args>
    class RenderCommand
    {
        friend class RenderQueue;

    public:
        RenderCommand() = default;
        RenderCommand(const std::string &name, RenderFunc<Args &&...> func, int32_t commandDepth, Shader *shader, Args &&...args)
            : m_name(name), m_func(func), m_commandDepth(commandDepth), m_shader(shader), m_args(std::forward<Args>(args)...)
        {
        }
        void Update(Camera *cam, WindowSystem *window, Scene *scene)
        {
            m_func(m_shader, cam, window, scene, std::forward<Args>(m_args)...);
        }
        ~RenderCommand() = default;

    private:
        std::string m_name;
        std::tuple<Args...> m_args;
        RenderFunc<Args...> m_func;
        int32_t m_commandDepth;
        Shader *m_shader;
    };
    // 渲染队列，用于存储渲染命令
    template <typename... Args>
    class RenderQueue
    {
    public:
        RenderQueue() = default;
        ~RenderQueue() = default;
        void AddRenderCommand(RenderCommand<Args...> command)
        {
            m_renderCommands.push_back(command);
        }
        // 根据渲染命令的名字删除某个渲染命令，使用find_if和lambda表达式
        void RemoveRenderCommand(const std::string &name)
        {
            auto iter = std::find_if(m_renderCommands.begin(), m_renderCommands.end(), [&name](const RenderCommand &command)
                                     { return command.m_name == name; });
            if (iter != m_renderCommands.end())
            {
                m_renderCommands.erase(iter);
            }
        }
        // 根据深度排序
        void Sort()
        {
            std::sort(m_renderCommands.begin(), m_renderCommands.end(), [](const RenderCommand &a, const RenderCommand &b)
                      { return a.m_commandDepth < b.m_commandDepth; });
        }
        // 更新渲染命令
        void Update(Camera *cam, WindowSystem *window, Scene *scene)
        {
            for (auto &command : m_renderCommands)
            {
                command.Update(cam, window, scene);
            }
        }

    private:
        std::vector<RenderCommand<Args...>> m_renderCommands;
    };
}