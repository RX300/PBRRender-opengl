#pragma once
// 包含glad和glfw相关头文件
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <utility>
#include "Camera.h"
#include "Input.h"
#include <iomanip> // 用于设置输出格式
namespace Renderer
{

    // template <typename Function, typename... Args>
    // auto genericInputCallback(Function &&functionName, Args &&...args)
    // {
    //     return [function = std::forward<Function>(functionName),
    //             arguments = std::make_tuple(std::forward<Args>(args)...)](GLFWwindow *window, const auto... inputArgs)
    //     {
    //         const auto ptr = static_cast<Input *>(glfwGetWindowUserPointer(window));
    //         if (function)
    //         {
    //             std::apply(function, std::tuple_cat(std::move(arguments), std::make_tuple(inputArgs...)));
    //         }
    //     };
    // }

    // Gross lambda to connect the Input singleton to GLFW callbacks
#define genericInputCallback(functionName)                                       \
    [](GLFWwindow *window, const auto... args) {                                 \
        const auto ptr = static_cast<Input *>(glfwGetWindowUserPointer(window)); \
        if (ptr->functionName)                                                   \
        {                                                                        \
            ptr->functionName(args...);                                          \
        }                                                                        \
    }

    // 创建窗口类，管理窗口的创建、销毁、设置等
    class WindowSystem
    {
    public:
        WindowSystem() noexcept = default;

        WindowSystem(WindowSystem &&) = default;
        WindowSystem(const WindowSystem &) = delete;
        WindowSystem &operator=(WindowSystem &&) = default;
        WindowSystem &operator=(const WindowSystem &) = delete;

        ~WindowSystem() = default;

        GLFWwindow *Init(unsigned int width, unsigned int height, const char *title);
        void Close() const
        {
            // 关闭窗口
            glfwSetWindowShouldClose(m_window, true);
        }

        void SetWindowPos(const std::size_t x, const std::size_t y) const
        {
            // 设置窗口位置
            glfwSetWindowPos(m_window, x, y);
        }
        void SwapBuffers() const
        {
            // 交换缓冲区
            glfwSwapBuffers(m_window);
        }

        void EnableCursor() const
        {
            // 显示鼠标
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        void DisableCursor() const
        {
            // 隐藏鼠标
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }

        void SetVsync(const bool vsync) const
        {
            // 设置垂直同步
            glfwSwapInterval(vsync);
        }

        void setAspectRatio(const unsigned int width, const unsigned int height) const
        {
            // 设置窗口纵横比
            glfwSetWindowAspectRatio(m_window, width, height);
        }

        void setAsCurrentContext() const
        {
            // 设置当前窗口为当前线程的主上下文
            glfwMakeContextCurrent(m_window);
        }
        auto ShouldClose() const noexcept { return m_shouldWindowClose; }
        auto IsCursorVisible() const noexcept { return m_showCursor; }
        Camera *getCamera() { return &camera; }
        auto GetXYOffset() const noexcept { return xyoffset; }
        GLFWwindow *GetWindow() const noexcept { return m_window; }
        // Returns the window's framebuffer dimensions in pixels {width, height}.
        std::pair<int, int> GetFramebufferDims() const
        {
            int width, height;
            glfwGetFramebufferSize(m_window, &width, &height);

            return {width, height};
        }
        // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
        // ---------------------------------------------------------------------------------------------------------
        void Update()
        {
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_ESCAPE))
            {
                Close();
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_LEFT_SHIFT))
            {
                EnableCursor();
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_LEFT_CONTROL))
            {
                DisableCursor();
            }
        }

    private:
        // glfw: whenever the window size changed (by OS or user resize) this callback function executes
        // ---------------------------------------------------------------------------------------------
        static void framebuffer_size_callback(GLFWwindow *window, int width, int height)
        {
            // make sure the viewport matches the new window dimensions; note that width and
            // height will be significantly larger than specified on retina displays.
            glViewport(0, 0, width, height);
        }

        // glfw: whenever the mouse scroll wheel scrolls, this callback is called
        // ----------------------------------------------------------------------
        static void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
        {
            // 使用glfwSetWindowsUserPointer()函数将窗口指针传递给回调函数
            auto *userwindow = static_cast<WindowSystem *>(glfwGetWindowUserPointer(window));
            userwindow->camera.ProcessMouseScroll(static_cast<float>(yoffset));
        }

    private:
        GLFWwindow *m_window = nullptr;
        bool m_shouldWindowClose = false;
        bool m_showCursor = false;
        // camera
        Camera camera{(glm::vec3(0.0f, 0.0f, 3.0f))};
        float lastX = 0;
        float lastY = 0;
        bool firstMouse = true;
        // mousemovement
        std::pair<float, float> xyoffset = {0.0f, 0.0f};
        // timing
        float deltaTime = 0.0f; // time between current frame and last frame
        float lastFrame = 0.0f;
    };
    // WindowSystem类的Init函数定义
    inline GLFWwindow *WindowSystem::Init(unsigned int width, unsigned int height, const char *title)
    {
        /*******glfw初始化*******/
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // 创建窗口
        m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (m_window == nullptr)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return nullptr;
        }
        // // 设置当前窗口为当前线程的主上下文
        glfwMakeContextCurrent(m_window);
        // disable vsync
        glfwSwapInterval(0);
        // 设置
        glfwSetWindowUserPointer(m_window, &Input::GetInstance());

        // 设置窗口大小改变时的回调函数
        glfwSetFramebufferSizeCallback(m_window, framebuffer_size_callback);
        // // 设置鼠标移动时的回调函数
        // glfwSetCursorPosCallback(m_window, genericInputCallback(Input::GetInstance().mouseMoved));
        // // 设置键盘按下时的回调函数
        // glfwSetKeyCallback(m_window, genericInputCallback(Input::GetInstance().keyPressed));
        // // 设置鼠标滚轮滚动时的回调函数
        // glfwSetScrollCallback(m_window, scroll_callback);
        // 告诉glfw捕获鼠标
        this->DisableCursor();
        lastX = width / 2.0f;
        lastY = height / 2.0f;
        return m_window;
    }
    // WindowSystem类的Update函数定义
    // void WindowSystem::Update()
    // {
    //     // per-frame time logic
    //     // --------------------
    //     float currentFrame = static_cast<float>(glfwGetTime());
    //     deltaTime = currentFrame - lastFrame;
    //     lastFrame = currentFrame;
    //     std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << std::fixed << 1.0f / deltaTime
    //               << "    currentFrame: " << std::setw(8) << std::setprecision(5) << std::fixed << currentFrame << std::flush;

    //     camera.Update(deltaTime);
    //     this->Update2();
    //     Input::GetInstance().Update();
    //     // // 输入
    //     // processInput(m_window);
    //     // 交换缓冲
    //     glfwSwapBuffers(m_window);
    //     // 检查是否有触发事件（键盘输入、鼠标移动等）
    //     glfwPollEvents();
    // }
}