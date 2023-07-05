#pragma once
#include "glad/glad.h"
#include <vector>
#include <ranges>
namespace Renderer
{
    class Framebuffer
    {
    public:
        Framebuffer()
        {
            glGenFramebuffers(1, &m_fbo);
            m_textures.resize(1);
            m_rbos.resize(1);
            // 生成一个纹理附件并将其附加到帧缓冲对象
            GLuint texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            m_textures.push_back(texture);
            // 生成一个渲染缓冲对象并将其附加到帧缓冲对象
            GLuint rbo;
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 800, 600);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
            m_rbos.push_back(rbo);
            // 检查帧缓冲是否完整
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        };
        ~Framebuffer()
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glDeleteFramebuffers(1, &m_fbo);
            glDeleteTextures(m_textures.size(), m_textures.data());
            glDeleteRenderbuffers(m_rbos.size(), m_rbos.data());
        };
        void bind() { glBindFramebuffer(GL_FRAMEBUFFER, m_fbo); };
        void unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); };
        void setRBO(unsigned int rbo_id, unsigned int dst_width, unsigned int dst_height)
        {
            GLuint rbo = m_rbos[rbo_id];
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, dst_width, dst_height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
        };
        GLuint addRBO(unsigned RBO_num, unsigned int dst_width, unsigned int dst_height, bool isbind)
        {
            GLuint rbo;
            glGenRenderbuffers(1, &rbo);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, dst_width, dst_height);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            if (isbind)
            {
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
            }

            m_rbos.push_back(rbo);
            return rbo;
        };
        // 禁止所有的复制拷贝移动操作
        Framebuffer(const Framebuffer &) = delete;
        Framebuffer &operator=(const Framebuffer &) = delete;
        Framebuffer(Framebuffer &&) = delete;
        Framebuffer &operator=(Framebuffer &&) = delete;

        unsigned int m_fbo;
        std::vector<GLuint> m_textures;
        std::vector<GLuint> m_rbos;
    };
}