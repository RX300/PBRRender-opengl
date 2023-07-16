#pragma once

#include <string_view>
#include <glad/glad.h>
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "Shader.h"
#include "filesystem.h"
#include "Framebuffer.h"
namespace Renderer
{
    class Skybox
    {

    public:
        // 从HDR贴图中加载立方体贴图初始化天空盒
        Skybox() = default;
        Skybox(const char *hdrPath, const std::size_t resolution, GLFWwindow *window);
        ~Skybox()
        {
            std::cout << "Skybox destructor called" << std::endl;
            // 删除hdr纹理
            if (m_isHdrTexture)
                glDeleteTextures(1, &m_hdrTexture);
            // 删除环境贴图
            if (m_isEnvCubemap)
                glDeleteTextures(1, &m_envCubeMap);
            // 删除辐照度贴图
            if (m_isIrradianceMap)
                glDeleteTextures(1, &m_irradianceMap);
            // 删除预过滤贴图
            if (m_isPrefilterMap)
                glDeleteTextures(1, &m_prefilterMap);
            // 删除BRDF LUT贴图
            if (m_isBrdfLUT)
                glDeleteTextures(1, &m_brdfLUT);
        }
        void Load(const char *hdrPath, const std::size_t resolution, GLFWwindow *window);
        void DrawSkybox(const glm::mat4 &view)
        {
            m_backgroundShader.use();
            m_backgroundShader.setMat4("view", view);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap);
            // glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap); // display irradiance map
            // glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap); // display prefilter map
            renderCube();
            m_backgroundShader.unuse();
        }
        void DrawPostProcess();
        // 烘焙IBL
        void bakeIBL(GLFWwindow *window)
        {
            m_irradianceMap = createIrradiancemap(32, window);
            m_prefilterMap = createPrefilterMap(128, 5, window);
            m_brdfLUT = createBrdfLUTMap(512, window);
        }
        // 烘焙辐照度贴图、预过滤贴图、BRDF LUT贴图
        GLuint createIrradiancemap(unsigned int size, GLFWwindow *window);
        GLuint createPrefilterMap(unsigned int baseMipSize, unsigned int maxMipLevel, GLFWwindow *window);
        GLuint createBrdfLUTMap(unsigned int size, GLFWwindow *window);
        // 设置
        void setProjection(glm::mat4 projection)
        {
            m_backgroundShader.use();
            m_backgroundShader.setMat4("projection", projection);
            m_backgroundShader.unuse();
        }
        void setCubeMap(unsigned int size);
        void loadCubeMapFromHDR(unsigned int hdrTexture, unsigned int size, GLFWwindow *window);
        void loadCubemap(std::vector<std::string> faces);
        //  获取环境贴图、辐照度贴图、预过滤贴图、BRDF LUT贴图
        auto *GetBackgroundShader() noexcept { return &m_backgroundShader; }
        auto GetHdrTexture() const noexcept { return m_hdrTexture; }
        auto GetEnvCubemap() const noexcept { return m_envCubeMap; }
        auto GetIrradianceMap() const noexcept { return m_irradianceMap; }
        auto GetPrefilterMap() const noexcept { return m_prefilterMap; }
        auto GetBRDFLUTMap() const noexcept { return m_brdfLUT; }

    private:
        void loadHdrTexture(const char *path);
        void renderCube();
        void renderQuad();
        GLuint m_hdrTexture;
        bool m_isHdrTexture = false;

        GLuint m_envCubeMap = 0;
        bool m_isEnvCubemap = false;
        GLuint m_irradianceMap = 0;
        bool m_isIrradianceMap = false;
        GLuint m_prefilterMap = 0;
        bool m_isPrefilterMap = false;
        GLuint m_brdfLUT = 0;
        bool m_isBrdfLUT = false;

        Framebuffer m_captureFBO;

        Shader m_skyboxShader{"skyboxShader"};
        Shader m_equirectangularToCubemapShader{"equirectangularToCubemapShader"};
        Shader m_irradianceShader{"irradianceShader"};
        Shader m_prefilterShader{"prefilterShader"};
        Shader m_brdfShader{"brdfShader"};
        Shader m_backgroundShader{"backgroundShader"};

        unsigned int m_cubeVAO = 0;
        unsigned int m_cubeVBO = 0;

        unsigned int quadVAO = 0;
        unsigned int quadVBO = 0;
    };

    inline Skybox::Skybox(const char *hdrPath, const std::size_t resolution, GLFWwindow *window)
    {
        loadHdrTexture(hdrPath);
        glGenTextures(1, &m_envCubeMap);
        m_isHdrTexture = true;
        setCubeMap(resolution);
        m_equirectangularToCubemapShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/equirectangular_to_cubemap.fs").c_str());
        m_irradianceShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/irradiance_convolution.fs").c_str());
        m_prefilterShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/prefilter.fs").c_str());
        m_brdfShader.loadShader(FileSystem::getPath("shader/IBL/brdf.vs").c_str(), FileSystem::getPath("shader/IBL/brdf.fs").c_str());
        loadCubeMapFromHDR(m_hdrTexture, resolution, window);
        m_backgroundShader.loadShader(FileSystem::getPath("shader/IBL/background.vs").c_str(), FileSystem::getPath("shader/IBL/background.fs").c_str());
        m_backgroundShader.use();
        m_backgroundShader.setInt("environmentMap", 0);
        m_backgroundShader.unuse();
    }
    inline void Skybox::Load(const char *hdrPath, const std::size_t resolution, GLFWwindow *window)
    {
        loadHdrTexture(hdrPath);
        glGenTextures(1, &m_envCubeMap);
        m_isHdrTexture = true;
        setCubeMap(resolution);
        m_equirectangularToCubemapShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/equirectangular_to_cubemap.fs").c_str());
        m_irradianceShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/irradiance_convolution.fs").c_str());
        m_prefilterShader.loadShader(FileSystem::getPath("shader/IBL/cubemap.vs").c_str(), FileSystem::getPath("shader/IBL/prefilter.fs").c_str());
        m_brdfShader.loadShader(FileSystem::getPath("shader/IBL/brdf.vs").c_str(), FileSystem::getPath("shader/IBL/brdf.fs").c_str());
        loadCubeMapFromHDR(m_hdrTexture, resolution, window);
        m_backgroundShader.loadShader(FileSystem::getPath("shader/IBL/background.vs").c_str(), FileSystem::getPath("shader/IBL/background.fs").c_str());
        m_backgroundShader.use();
        m_backgroundShader.setInt("environmentMap", 0);
        m_backgroundShader.unuse();
    }
    inline GLuint Skybox::createIrradiancemap(unsigned int size, GLFWwindow *window)
    { // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
        // --------------------------------------------------------------------------------
        unsigned int irradianceMap;
        glGenTextures(1, &irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        m_captureFBO.bind();
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureFBO.m_rbos[0]);
        // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
        // -----------------------------------------------------------------------------
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
            {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

        m_irradianceShader.use();
        m_irradianceShader.setInt("environmentMap", 0);
        m_irradianceShader.setMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap);

        glViewport(0, 0, size, size); // don't forget to configure the viewport to the capture dimensions.
        for (unsigned int i = 0; i < 6; ++i)
        {
            m_irradianceShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();
        }
        //
        std::cout << "irradianceMap:" << irradianceMap << std::endl;
        m_isIrradianceMap = true;
        // 解除绑定
        m_captureFBO.unbind();
        m_irradianceShader.unuse();
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        // then before rendering, configure the viewport to the original framebuffer's screen dimensions
        int scrWidth, scrHeight;
        glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
        glViewport(0, 0, scrWidth, scrHeight);

        m_irradianceMap = irradianceMap;
        return irradianceMap;
    }
    inline GLuint Skybox::createPrefilterMap(unsigned int baseMipSize, unsigned int maxMipLevel, GLFWwindow *window)
    {
        // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
        // --------------------------------------------------------------------------------
        unsigned int prefilterMap;
        glGenTextures(1, &prefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, baseMipSize, baseMipSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
        // ----------------------------------------------------------------------------------------------------
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] =
            {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};
        m_prefilterShader.use();
        m_prefilterShader.setInt("environmentMap", 0);
        m_prefilterShader.setMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap);

        m_captureFBO.bind();
        unsigned int maxMipLevels = maxMipLevel;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
        {
            // reisze framebuffer according to mip-level size.
            unsigned int mipWidth = static_cast<unsigned int>(baseMipSize * std::pow(0.5, mip));
            unsigned int mipHeight = static_cast<unsigned int>(baseMipSize * std::pow(0.5, mip));
            glBindRenderbuffer(GL_RENDERBUFFER, m_captureFBO.m_rbos[0]);

            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            m_prefilterShader.setFloat("roughness", roughness);
            for (unsigned int i = 0; i < 6; ++i)
            {
                m_prefilterShader.setMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                renderCube();
            }
        }
        //
        std::cout << "prefilterMap:" << prefilterMap << std::endl;
        m_isPrefilterMap = true;
        // 解除绑定
        m_captureFBO.unbind();
        m_prefilterShader.unuse();
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        // then before rendering, configure the viewport to the original framebuffer's screen dimensions
        int scrWidth, scrHeight;
        glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
        glViewport(0, 0, scrWidth, scrHeight);

        m_prefilterMap = prefilterMap;
        return prefilterMap;
    }
    inline GLuint Skybox::createBrdfLUTMap(unsigned int size, GLFWwindow *window)
    {
        // pbr: generate a 2D LUT from the BRDF equations used.
        // ----------------------------------------------------
        unsigned int brdfLUTTexture;
        glGenTextures(1, &brdfLUTTexture);

        // pre-allocate enough memory for the LUT texture.
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, 0);
        // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
        m_captureFBO.bind();
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureFBO.m_rbos[0]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

        glViewport(0, 0, size, size);
        m_brdfShader.use();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderQuad();
        //
        std::cout << "brdfLUTTexture:" << brdfLUTTexture << std::endl;
        m_isBrdfLUT = true;
        // 解除绑定
        m_brdfShader.unuse();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // then before rendering, configure the viewport to the original framebuffer's screen dimensions
        int scrWidth, scrHeight;
        glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
        glViewport(0, 0, scrWidth, scrHeight);

        return brdfLUTTexture;
    }

    inline void Skybox::setCubeMap(unsigned int size)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_envCubeMap);
        for (unsigned int i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, size, size, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }
    inline void Skybox::loadCubeMapFromHDR(unsigned int hdrTexture, unsigned int size, GLFWwindow *window)
    {
        // pbr:设置投影和视图矩阵，以便在6个立方体贴图面方向上捕获数据
        //  ----------------------------------------------------------------------------------------------
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

        m_equirectangularToCubemapShader.use();
        m_equirectangularToCubemapShader.setInt("equirectangularMap", 0);
        m_equirectangularToCubemapShader.setMat4("projection", captureProjection);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glViewport(0, 0, size, size); // don't forget to configure the viewport to the capture dimensions.

        glm::mat4 captureViews[] =
            {
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
                glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

        // 绑定帧缓冲
        m_captureFBO.bind();
        glBindRenderbuffer(GL_RENDERBUFFER, m_captureFBO.m_rbos[0]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_equirectangularToCubemapShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_envCubeMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            renderCube();
        }
        //
        std::cout << "envCubeMap:" << m_envCubeMap << std::endl;
        m_isEnvCubemap = true;
        // 解除绑定
        m_equirectangularToCubemapShader.unuse();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        // then before rendering, configure the viewport to the original framebuffer's screen dimensions
        int scrWidth, scrHeight;
        glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
        glViewport(0, 0, scrWidth, scrHeight);
    }
    inline void Skybox::loadCubemap(std::vector<std::string> faces)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        m_envCubeMap = textureID;
        //
        std::cout << "envCubeMap:" << m_envCubeMap << std::endl;
        m_isEnvCubemap = true;
    }
    inline void Skybox::loadHdrTexture(const char *path)
    {
        stbi_set_flip_vertically_on_load(true);
        int width, height, nrComponents;
        // 加载hdr图片，要用stbi_loadf函数将其加载为一个浮点数数组
        float *data = stbi_loadf(path, &width, &height, &nrComponents, 0);
        unsigned int hdrTexture;
        if (data)
        {
            glGenTextures(1, &hdrTexture);
            glBindTexture(GL_TEXTURE_2D, hdrTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

            // 设置纹理环绕方式以及过滤方式
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Failed to load HDR image" << std::endl;
        }
        m_hdrTexture = hdrTexture;
        m_isHdrTexture = true;
    }
    inline void Skybox::renderCube()
    {
        // initialize (if necessary)
        if (m_cubeVAO == 0)
        {
            float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
                1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
                1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,   // top-right
                -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,  // top-left
                // front face
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,  // bottom-right
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
                1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,   // top-right
                -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // top-left
                -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
                -1.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,  // top-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
                -1.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,   // top-right
                                                                    // right face
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,     // top-left
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // bottom-right
                1.0f, 1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,    // top-right
                1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,   // bottom-right
                1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,     // top-left
                1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,    // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,  // top-left
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
                1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,   // bottom-left
                -1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // bottom-right
                -1.0f, -1.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
                1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // top-right
                1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,   // bottom-right
                -1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // top-left
                -1.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f   // bottom-left
            };
            glGenVertexArrays(1, &m_cubeVAO);
            glGenBuffers(1, &m_cubeVBO);
            // fill buffer
            glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            // link vertex attributes
            glBindVertexArray(m_cubeVAO);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
        }
        // render Cube
        glBindVertexArray(m_cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }
    // renderQuad() renders a 1x1 XY quad in NDC
    // -----------------------------------------

    inline void Skybox::renderQuad()
    {
        if (quadVAO == 0)
        {
            float quadVertices[] = {
                // positions        // texture Coords
                -1.0f,
                1.0f,
                0.0f,
                0.0f,
                1.0f,
                -1.0f,
                -1.0f,
                0.0f,
                0.0f,
                0.0f,
                1.0f,
                1.0f,
                0.0f,
                1.0f,
                1.0f,
                1.0f,
                -1.0f,
                0.0f,
                1.0f,
                0.0f,
            };
            // setup plane VAO
            glGenVertexArrays(1, &quadVAO);
            glGenBuffers(1, &quadVBO);
            glBindVertexArray(quadVAO);
            glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
        }
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
}
