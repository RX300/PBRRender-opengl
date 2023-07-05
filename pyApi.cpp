#pragma once
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

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
using namespace Renderer;

inline pybind11::array_t<unsigned char> PBRRender::ReadCurFrameBufferToNumpy()
{
    auto resolution = m_window.GetFramebufferDims();
    // 绑定PBO
    glBindBuffer(GL_PIXEL_PACK_BUFFER, readPBO);
    glReadPixels(0, 0, resolution.first, resolution.second, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    // 将帧缓冲区数据复制到NumPy数组中
    auto frameBufferData = static_cast<unsigned char *>(glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY));

    // 取消映射并解绑PBO
    glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    // 创建NumPy数组，不分配内存
    pybind11::array_t<unsigned char> img({resolution.second, resolution.first, 4}, {resolution.first * 4, 4, 1}, frameBufferData);
    return img;
}

PYBIND11_MODULE(renderPyApi, m)
{
    m.doc() = "pybind11 render api"; // optional module docstring
    // 定义glm::vec3类
    pybind11::class_<glm::vec3>(m, "glmvec3", "glm::vec3 class")
        .def(pybind11::init<float, float, float>(), pybind11::arg("x") = 0.0f, pybind11::arg("y") = 0.0f, pybind11::arg("z") = 0.0f, "glm::vec3 constructor")
        .def_readwrite("x", &glm::vec3::x)
        .def_readwrite("y", &glm::vec3::y)
        .def_readwrite("z", &glm::vec3::z);
    // 定义Camera类，只需要定义构造函数和获取指针类即可
    pybind11::class_<Camera>(m, "Camera", "Camera class")
        .def(pybind11::init<glm::vec3, glm::vec3, float, float>(), pybind11::arg("position") = glm::vec3(0.0f, 0.0f, 0.0f), pybind11::arg("up") = glm::vec3(0.0f, 1.0f, 0.0f), pybind11::arg("yaw") = YAW, pybind11::arg("pitch") = PITCH, "Camera constructor")
        .def("GetCameraPtr", &Camera::GetCameraPtr, "Get the pointer of camera");
    //  定义PBRRender类
    pybind11::class_<PBRRender>(m, "PBRRender", "A class of Renderer")
        .def(pybind11::init<>())
        .def("LoadScene", &PBRRender::LoadScene, "Load a scene")
        .def("LoadCamera", &PBRRender::LoadCamera, "Load a camera")
        // .def("GetRenderQueue", &PBRRender::GetRenderQueue, "Get the render queue")
        // .def("GetInitQueue", &PBRRender::GetInitQueue, "Get the init queue")
        // .def("GetCurrentWindow", &PBRRender::GetCurrentWindow, "Get the current window")
        .def("Init", &PBRRender::Init, "Init the renderer")
        .def("RenderTest", &PBRRender::RenderTest, "Render the scene")
        .def("RenderTestInit", &PBRRender::RenderTestInit, "Init the render test")
        .def("RenderTestUpdate", &PBRRender::RenderTestUpdate, "Update the render test")
        .def("RenderTestShouldClose", &PBRRender::RenderTestShouldClose, "Check if the render test should close")
        .def("ReadCurFrameBufferToNumpy", &PBRRender::ReadCurFrameBufferToNumpy, "Read the current framebuffer to numpy array");
}