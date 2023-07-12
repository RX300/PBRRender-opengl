#pragma once
#include "Input.h"
#include "glad/glad.h"
#include "RenderQueue.h"
#include "GBuffer.h"
#include <pybind11/numpy.h>
namespace Renderer
{
    inline void pbrShaderInit(Shader *shader, WindowSystem *window, Camera *cam, Scene *scene)
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
    }
    // PBRRender类
    class PBRRender
    {
    public:
        PBRRender() = default;
        ~PBRRender();
        void Init(unsigned int width, unsigned int height, const char *title);
        void Render(Shader &shader);
        void RenderTestInit()
        {
            glEnable(GL_DEPTH_TEST);
            // set depth function to less than AND equal for skybox depth trick.
            glDepthFunc(GL_LEQUAL);
            // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
            glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
            // 初始化计数器和计时器
            frameCount = 0;
            timer = glfwGetTime();
        }
        void RenderTestUpdate()
        {
            // per-frame time logic
            // --------------------
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            // std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << std::fixed << 1.0f / deltaTime
            //           << "    currentFrame: " << std::setw(8) << std::setprecision(5) << std::fixed << currentFrame << std::flush;
            // 渲染指令
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_camera->Update(deltaTime);
            m_window.Update();
            Input::GetInstance().Update();
            // 交换缓冲
            m_window.SwapBuffers();
            // 检查是否有触发事件（键盘输入、鼠标移动等）
            glfwPollEvents();
            // 更新计数器
            frameCount++;
            // 当达到一秒时，打印帧数并重置计数器和计时器
            if (glfwGetTime() - timer >= 1.0)
            {
                std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << frameCount << std::flush;
                frameCount = 0;
                timer = glfwGetTime();
            }
        }
        bool RenderTestShouldClose()
        {
            return glfwWindowShouldClose(m_window.GetWindow());
        }
        unsigned char *ReadCurFrameBuffer()
        {
            auto resolution = m_window.GetFramebufferDims();
            // 绑定PBO
            glBindBuffer(GL_PIXEL_PACK_BUFFER, readPBO);
            // 默认情况下，glReadPixels 函数会读取后台缓冲区（back buffer）中的像素数据。但是，如果你想读取前台缓冲区（front buffer）中的像素数据，就需要先调用 glReadBuffer(GL_FRONT) 函数来设置读取颜色缓冲区的方式。但是双缓冲的情况下，前台缓冲区的内容是不确定的，所以这种方式并不可靠。
            // glReadBuffer(GL_FRONT);
            //  读取帧缓冲区数据到PBO(最后一个参数是内存地址指针，当等于0时就是从帧缓冲区往绑定的PBO传输数据)
            glReadPixels(0, 0, resolution.first, resolution.second, GL_RGBA, GL_UNSIGNED_BYTE, 0);
            unsigned char *data = new unsigned char[resolution.first * resolution.second * 4];
            auto FrameBufferData = (unsigned char *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
            return FrameBufferData;
        }
        // pybind11::array_t<unsigned char> ReadCurFrameBufferToNumpy();
        // void RenderTest()
        // {
        //     glEnable(GL_DEPTH_TEST);
        //     // set depth function to less than AND equal for skybox depth trick.
        //     glDepthFunc(GL_LEQUAL);
        //     // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
        //     glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        //     m_renderQueue.Sort();
        //     // 渲染循环
        //     while (!glfwWindowShouldClose(m_window.GetWindow()))
        //     {
        //         // per-frame time logic
        //         // --------------------
        //         float currentFrame = static_cast<float>(glfwGetTime());
        //         deltaTime = currentFrame - lastFrame;
        //         lastFrame = currentFrame;
        //         // std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << std::fixed << 1.0f / deltaTime
        //         //           << "    currentFrame: " << std::setw(8) << std::setprecision(5) << std::fixed << currentFrame << std::flush;
        //         // 渲染指令
        //         glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        //         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //         m_camera->Update(deltaTime);
        //         m_window.Update();
        //         Input::GetInstance().Update();
        //         // 交换缓冲
        //         m_window.SwapBuffers();
        //         // 检查是否有触发事件（键盘输入、鼠标移动等）
        //         glfwPollEvents();
        //         // 更新计数器
        //         frameCount++;

        //         // 当达到一秒时，打印帧数并重置计数器和计时器
        //         if (glfwGetTime() - timer >= 1.0)
        //         {
        //             std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << frameCount << std::flush;
        //             frameCount = 0;
        //             timer = glfwGetTime();
        //         }
        //     }
        // }

        // void Update();
        void LoadScene(Scene *scene) { m_scene = scene; }
        void LoadCamera(Camera *camera) { m_camera = camera; }
        void SetWindow(WindowSystem &Window) { Window.setAsCurrentContext(); }

        auto *GetInitQueue() { return &m_initQueue; };
        auto *GetRenderQueue() { return &m_renderQueue; };
        auto *GetPostQueue() { return &m_postQueue; };
        auto *GetCurrentWindow() { return m_window.GetWindow(); }

    private:
        GLuint readPBO;
        WindowSystem m_window;
        Camera *m_camera;
        Scene *m_scene;
        // timing
        float deltaTime = 0.0f; // time between current frame and last frame
        float lastFrame = 0.0f;
        // 初始化计数器和计时器
        int frameCount = 0;
        double timer = 0;
        // 渲染队列,分为初始队列，渲染队列，后处理队列
        RenderQueue m_initQueue;
        RenderQueue m_renderQueue;
        RenderQueue m_postQueue;
    };
    inline PBRRender::~PBRRender()
    {
        glfwTerminate();
    }
    inline void PBRRender::Init(unsigned int width, unsigned int height, const char *title)
    {
        m_window.Init(width, height, title);
        // 设置鼠标移动时的回调函数
        glfwSetCursorPosCallback(m_window.GetWindow(), genericInputCallback(Input::GetInstance().mouseMoved));
        // 设置键盘按下时的回调函数
        glfwSetKeyCallback(m_window.GetWindow(), genericInputCallback(Input::GetInstance().keyPressed));
        /*******glad初始化，在这之前不能使用opengl的函数*******/
        // GLAD是用来管理OpenGL的函数指针的，
        // 所以在调用任何OpenGL的函数之前我们需要初始化GLAD。
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        {
            std::cout << "Failed to initialize GLAD" << std::endl;
        }
        else
        {
            std::cout << "glad init success" << std::endl;
        }
        glViewport(0, 0, width, height);

        // 创建PBO
        // glGenBuffers(1, &readPBO);
        // glBindBuffer(GL_PIXEL_UNPACK_BUFFER, readPBO);
        // glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_STREAM_DRAW);
        // glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    inline void PBRRender::Render(Shader &pbrShader)
    {
        // 传统调用着色器更新着色操作的方式
        //-------------------------------------------------------------------------------------------
        // pbrShader.use();
        // pbrShader.setInt("irradianceMap", 0);
        // pbrShader.setInt("prefilterMap", 1);
        // pbrShader.setInt("brdfLUT", 2);
        // pbrShader.setInt("material.albedoMap", 3);
        // pbrShader.setInt("material.normalMap", 4);
        // pbrShader.setInt("material.metallicMap", 5);
        // pbrShader.setInt("material.roughnessMap", 6);
        // pbrShader.setInt("material.aoMap", 7);
        // pbrShader.setVec3("material.albedo", 0.5f, 0.0f, 0.0f);
        // pbrShader.setFloat("ao", 1.0f);

        // auto resolution = m_window.GetFramebufferDims();
        // glm::mat4 projection = glm::perspective(glm::radians(m_camera->Zoom), (float)resolution.first / (float)resolution.second, 0.1f, 100.0f);
        // pbrShader.setMat4("projection", projection);
        // pbrShader.unuse();
        // m_scene->SetSkybox(projection);
        // m_scene->Bake(m_window.GetWindow());

        // 使用回调函数更新着色器
        //-------------------------------------------------------------------------------------------
        // pbrShader.initShaderRender(pbrShaderInit, &this->m_window, this->m_camera, this->m_scene);

        // 使用渲染队列更新着色器
        //-------------------------------------------------------------------------------------------
        m_initQueue.Sort();
        m_initQueue.Update(this->m_camera, &this->m_window, this->m_scene);
        /*******开启测试*******/
        // configure global opengl state
        // -----------------------------
        glEnable(GL_DEPTH_TEST);
        // set depth function to less than AND equal for skybox depth trick.
        glDepthFunc(GL_LEQUAL);
        // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        m_renderQueue.Sort();
        // 渲染循环
        while (!glfwWindowShouldClose(m_window.GetWindow()))
        {
            // per-frame time logic
            // --------------------
            float currentFrame = static_cast<float>(glfwGetTime());
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
            std::cout << "\rfps: " << std::setw(6) << std::setprecision(2) << std::fixed << 1.0f / deltaTime
                      << "    currentFrame: " << std::setw(8) << std::setprecision(5) << std::fixed << currentFrame << std::flush;
            // 渲染指令
            glClearColor(0.9f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            // 渲染场景
            m_scene->Update(pbrShader, *m_camera);
            m_renderQueue.Update(this->m_camera, &this->m_window, this->m_scene);
            // 摄像机系统，窗口系统，输入控制系统更新
            m_camera->Update(deltaTime);
            m_window.Update();
            Input::GetInstance().Update();
            // 交换缓冲
            m_window.SwapBuffers();
            // 检查是否有触发事件（键盘输入、鼠标移动等）
            glfwPollEvents();
        }
    }

} // namespace Renderer