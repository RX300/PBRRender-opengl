#pragma once

#include <string_view>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "filesystem.h"
namespace Renderer
{
    class GBuffer
    {
    public:
        GBuffer()
        {
        }
        GBuffer(unsigned int width, unsigned int height)
        {
            // 创建g-buffer
            // glGenFramebuffers(1, &m_gBuffer);
            // glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
            // glBindFramebuffer(GL_FRAMEBUFFER, 0);
            // m_GbufferGeometryPass.loadShader("GbufferGeometryPass", FileSystem::getPath("shader/G-Buffer/g_buffer.vs").c_str(), FileSystem::getPath("shader/G-Buffer/g_buffer.fs").c_str());
            // m_GbufferLightingPass.loadShader("GbufferLightingPass", FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.vs").c_str(), FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.fs").c_str());
            Load(width, height);
        }
        ~GBuffer()
        {
            glDeleteFramebuffers(1, &m_gBuffer);
            glDeleteTextures(1, &m_gPositionRoughness);
            glDeleteTextures(1, &m_gNormalAO);
            glDeleteTextures(1, &m_gAlbedoMetallic);
            glDeleteTextures(1, &m_gEmission);
        }
        void Load(unsigned int width, unsigned int height)
        {
            glGenFramebuffers(1, &m_gBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            m_GbufferGeometryPass.loadShader("GbufferGeometryPass", FileSystem::getPath("shader/G-Buffer/g_buffer.vs").c_str(), FileSystem::getPath("shader/G-Buffer/g_buffer.fs").c_str());
            m_GbufferLightingPass.loadShader("GbufferLightingPass", FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.vs").c_str(), FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.fs").c_str());

            m_width = width;
            m_height = height;
            glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer);
            // position color buffer + Roughness(每分量16位float)
            glGenTextures(1, &m_gPositionRoughness);
            glBindTexture(GL_TEXTURE_2D, m_gPositionRoughness);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_gPositionRoughness, 0);
            // normal color buffer + AO(每分量16位float)
            glGenTextures(1, &m_gNormalAO);
            glBindTexture(GL_TEXTURE_2D, m_gNormalAO);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_gNormalAO, 0);
            // color + specular color buffer
            // 这里只使用了一个颜色缓冲，我们将颜色和镜面强度数据合并到一起，存储到一个单独的RGBA纹理里面(每分量8位float)
            glGenTextures(1, &m_gAlbedoMetallic);
            glBindTexture(GL_TEXTURE_2D, m_gAlbedoMetallic);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_gAlbedoMetallic, 0);
            // // Emission
            // glGenTextures(1, &m_gEmission);
            // glBindTexture(GL_TEXTURE_2D, m_gEmission);
            // glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, NULL);
            // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
            unsigned int attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
            glDrawBuffers(3, attachments);
            // create and attach depth buffer (renderbuffer)
            unsigned int rboDepth;
            glGenRenderbuffers(1, &rboDepth);
            glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
            // finally check if framebuffer is complete
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "G-Buffer not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // shader设置
            m_GbufferGeometryPass.use();
            m_GbufferGeometryPass.setInt("material.albedoMap", 3);
            m_GbufferGeometryPass.setInt("material.normalMap", 4);
            m_GbufferGeometryPass.setInt("material.metallicMap", 5);
            m_GbufferGeometryPass.setInt("material.roughnessMap", 6);
            m_GbufferGeometryPass.setInt("material.aoMap", 7);
            m_GbufferGeometryPass.unuse();

            m_GbufferLightingPass.use();
            m_GbufferLightingPass.setInt("gPositionRoughness", 0);
            m_GbufferLightingPass.setInt("gNormalAO", 1);
            m_GbufferLightingPass.setInt("gAlbedoMetallic", 2);
            // m_GbufferLightingPass.setInt("gEmission", 3);
            m_GbufferLightingPass.unuse();
        }
        void Render();
        // g-buffer framebuffer
        unsigned int m_gBuffer;
        // 要保存的g-buffer的纹理
        unsigned int m_gPositionRoughness, m_gNormalAO;
        unsigned int m_gAlbedoMetallic;
        unsigned int m_gEmission;

        unsigned int m_width, m_height;

        // shader
        // Shader m_GbufferGeometryPass{"GbufferGeometryPass", FileSystem::getPath("shader/G-Buffer/g_buffer.vs").c_str(), FileSystem::getPath("shader/G-Buffer/g_buffer.fs").c_str()};
        // Shader m_GbufferLightingPass{"GbufferLightingPass", FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.vs").c_str(), FileSystem::getPath("shader/G-Buffer/deferred_shadingPBR.fs").c_str()};

        Shader m_GbufferGeometryPass;
        Shader m_GbufferLightingPass;
    };
    // // 静态成员初始化
    // Shader GBuffer::GbufferGeometryPass.loadShader("GbufferGeometryPass", "shader/G-buffer/g_buffer.vs", "shader/G-buffer/g_buffer.fs");
    // Shader GBuffer::GbufferLightingPass.loadShader("GbufferLightingPass", "shader/G-buffer/deferred_shadingPBR.vs", "shader/G-buffer/deferred_shadingPBR.fs");

    inline void GBuffer::Render()
    {
    }
}