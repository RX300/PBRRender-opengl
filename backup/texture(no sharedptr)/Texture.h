#pragma once
#include <glad/glad.h>
#include <stb_image.h>
// 用raii的方式管理纹理，但是注意要删除拷贝构造函数和赋值运算符，具体原因见下面的链接
// https://www.khronos.org/opengl/wiki/Common_Mistakes#RAII_and_hidden_destructor_calls
namespace Renderer
{
    // 纹理类，一个纹理对象包含了一个纹理ID，一个纹理类型，一个纹理格式，宽高度，以及纹理数据
    class Texture
    {
    public:
        Texture() noexcept { std::cout << "Texture created" << std::endl; };
        // 删除拷贝构造函数
        Texture(const Texture &) = delete;
        // 删除赋值运算符
        Texture &operator=(const Texture &) = delete;
        // 定义移动构造函数
        Texture(Texture &&other) : id(other.id), type(other.type), path(other.path), loaded(other.loaded)
        {
            std::cout << "Texture: " << path << " moved" << std::endl;
        }
        // 定义移动赋值运算符
        Texture &operator=(Texture &&other)
        {
            if (this != &other)
            {
                id = other.id;
                type = other.type;
                path = other.path;
                loaded = other.loaded;
                other.loaded = false;
            }
            std::cout << "Texture: " << path << " moved" << std::endl;
            return *this;
        }
        Texture(const char *filepath, bool gammaCorrection = false)
        {
            LoadTexture(filepath, gammaCorrection);
            std::cout << "Texture: " << path << " created" << std::endl;
        };
        Texture(const char *filepath, const std::string &directory, bool gammaCorrection = false)
        {
            TextureFromFile(filepath, directory, gammaCorrection);
            std::cout << "Texture: " << path << " created" << std::endl;
        }

        ~Texture()
        {
            if (loaded)
            {
                glDeleteTextures(1, &id);
                std::cout << "Texture: " << path << " deleted" << std::endl;
            }
        };

    private:
        void LoadTexture(char const *filepath, bool gammaCorrection = false);
        void TextureFromFile(const char *filepath, const std::string &directory, bool gammaCorrection = false);

    public:
        void SetTextureType(const std::string &type) { this->type = type; }
        unsigned int GetTextureID() const
        {
#ifdef _DEBUG
            assert(loaded);
#endif
            return id;
        }
        unsigned int id;
        std::string type;
        std::string path;
        bool loaded = false;
    };

    void Texture::LoadTexture(char const *filepath, bool gammaCorrection)
    {
        this->path = filepath;
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(filepath, &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum internalFormat;
            GLenum dataFormat;
            if (nrComponents == 1)
            {
                internalFormat = dataFormat = GL_RED;
            }
            else if (nrComponents == 3)
            {
                internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
                dataFormat = GL_RGB;
            }
            else if (nrComponents == 4)
            {
                internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
                dataFormat = GL_RGBA;
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            loaded = true;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
        }

        id = textureID;
    }
    void Texture::TextureFromFile(const char *filepath, const std::string &directory, bool gammaCorrection)
    {
        std::string filename = std::string(filepath);
        filename = directory + '/' + filename;
        path = filename;
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum internalFormat;
            GLenum dataFormat;
            if (nrComponents == 1)
            {
                internalFormat = dataFormat = GL_RED;
            }
            else if (nrComponents == 3)
            {
                internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
                dataFormat = GL_RGB;
            }
            else if (nrComponents == 4)
            {
                internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
                dataFormat = GL_RGBA;
            }

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
            loaded = true;
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
            glDeleteTextures(1, &textureID);
        }

        id = textureID;
    }
}