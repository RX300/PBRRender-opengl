#pragma once
#define STB_IMAGE_IMPLEMENTATION
// shader.h引用了glad.h，因此必须放在最前面
// 原因见https://blog.csdn.net/qq_40079310/article/details/114966609

#include "PBRRender.h"
#include "Framebuffer.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <iomanip> // 用于设置输出格式
#include <random>
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "filesystem.h"
#include "command.h"
using namespace Renderer;
using namespace ModelLoader;
// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const float near_plane = 0.1f;
const float far_plane = 100.0f;
bool bloom = true;
bool bloomKeyPressed = false;
float exposure = 1.0f;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));

float ourLerp(float a, float b, float f)
{
    return a + f * (b - a);
}

int main()
{
    Renderer::PBRRender pbrRender;
    pbrRender.Init(SCR_WIDTH, SCR_HEIGHT, "PBRRenderer");

    // /******创建shader*******/
    // 要注意相对路径的起点是exe启动的地方
    // build and compile shaders
    // -------------------------

    // PBR
    Shader pbrShader("pbrshader", "../../shader/PBR/pbr.vs", "../../shader/PBR/pbr.fs");
    Shader lightShader("lightboxShader", "../../shader/light/light_box.vs", "../../shader/light/light_box.fs");
    // // 将pbrShader将入到渲染命令中
    // Renderer::RenderCommand InitRenderCommand("PBRShaderInit", pbrInitFunc, 1000, pbrShader.getShaderPtr());
    // auto initQueue = pbrRender.GetInitQueue();
    // initQueue->AddRenderCommand(InitRenderCommand);

    // Renderer::RenderCommand LoopRenderCommand("PBRShaderRender", pbrRenderFunc, 1000, pbrShader.getShaderPtr());
    // auto renderQueue = pbrRender.GetRenderQueue();
    // renderQueue->AddRenderCommand(LoopRenderCommand);

    // 延迟着色
    // 将pbrShader将入到渲染命令中
    Renderer::RenderCommand InitRenderCommand("deferredInitFunc", deferredInitFunc, 1000, gBuffer.m_GbufferGeometryPass.getShaderPtr());
    Renderer::RenderCommand InitLightRenderCommand("lightBoxInitFunc", lightBoxInitFunc, 2000, lightShader.getShaderPtr());
    auto initQueue = pbrRender.GetInitQueue();
    initQueue->AddRenderCommand(InitRenderCommand);
    initQueue->AddRenderCommand(InitLightRenderCommand);

    Renderer::RenderCommand LoopRenderCommand1("deferredRenderGeometryFunc", deferredRenderGeometryFunc, 1000, gBuffer.m_GbufferGeometryPass.getShaderPtr());
    Renderer::RenderCommand LoopRenderCommand2("deferredRenderShaderFunc", deferredRenderShaderFunc, 2000, gBuffer.m_GbufferLightingPass.getShaderPtr());
    Renderer::RenderCommand LoopLightRenderCommand("lightBoxShaderFunc", lightBoxShaderFunc, 3000, lightShader.getShaderPtr());

    auto renderQueue = pbrRender.GetRenderQueue();
    renderQueue->AddRenderCommand(LoopRenderCommand1);
    renderQueue->AddRenderCommand(LoopRenderCommand2);
    renderQueue->AddRenderCommand(LoopLightRenderCommand);

    Renderer::Scene scene("testScene");
    Model helmetModel(FileSystem::getPath("pbr/DamagedHelmet/glTF/DamagedHelmet.gltf").c_str(), true, true);
    scene.AddModel(std::make_shared<Model>(helmetModel));
    // Model gunModel(FileSystem::getPath("pbr/gltf_Cerberus_low/Cerberus_LP.gltf").c_str(), true, true);
    // scene.AddModel(std::make_shared<Model>(gunModel));

    // Skybox
    //------
    // scene.LoadSkybox(FileSystem::getPath("newport_loft.hdr").c_str(), 4096, pbrRender.GetCurrentWindow());

    pbrRender.LoadScene(scene.GetScenePtr());
    pbrRender.LoadCamera(camera.GetCameraPtr());

    pbrRender.Render(pbrShader);

    return 0;
}
