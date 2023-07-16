#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
namespace Renderer
{
    size_t Typesize(GLenum type);

    class Shader
    {
    public:
        // name of the shader
        std::string m_name;
        // the program ID
        unsigned int ID;

        Shader(const std::string str) noexcept { m_name = str; }
        // constructor generates the shader on the fly
        // ------------------------------------------------------------------------
        Shader(const std::string str, const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr)
        {
            m_name = str;
            loadShader(vertexPath, fragmentPath, geometryPath);
        }
        // 获取当前类的指针
        Shader *getShaderPtr()
        {
            return this;
        }
        template <typename Func, typename... Args>
        void initShaderRender(Func func, Args &&...args)
        {
            auto lambda = [this, func](Args &&...args)
            {
                func(this, std::forward<Args>(args)...);
            };
            lambda(std::forward<Args>(args)...);
        }
        template <typename Func, typename... Args>
        void loopShaderRender(Func func, Args &&...args)
        {
            // func(std::forward<Args>(args)...);
            auto lambda = [this, func](Args &&...args)
            {
                func(this, std::forward<Args>(args)...);
            };
            lambda(std::forward<Args>(args)...);
        }
        void loadShader(const char *vertexPath, const char *fragmentPath, const char *geometryPath = nullptr)
        {
            // 1. retrieve the vertex/fragment source code from filePath
            std::string vertexCode;
            std::string fragmentCode;
            std::string geometryCode;
            std::ifstream vShaderFile;
            std::ifstream fShaderFile;
            std::ifstream gShaderFile;
            // ensure ifstream objects can throw exceptions:
            vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
            try
            {
                // open files
                vShaderFile.open(vertexPath);
                fShaderFile.open(fragmentPath);
                std::stringstream vShaderStream, fShaderStream;
                // read file's buffer contents into streams
                vShaderStream << vShaderFile.rdbuf();
                fShaderStream << fShaderFile.rdbuf();
                // close file handlers
                vShaderFile.close();
                fShaderFile.close();
                // convert stream into string
                vertexCode = vShaderStream.str();
                fragmentCode = fShaderStream.str();
                // if geometry shader path is present, also load a geometry shader
                if (geometryPath != nullptr)
                {
                    gShaderFile.open(geometryPath);
                    std::stringstream gShaderStream;
                    gShaderStream << gShaderFile.rdbuf();
                    gShaderFile.close();
                    geometryCode = gShaderStream.str();
                }
            }
            catch (std::ifstream::failure &e)
            {
                std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
            }
            const char *vShaderCode = vertexCode.c_str();
            const char *fShaderCode = fragmentCode.c_str();
            // 2. compile shaders
            unsigned int vertex, fragment;
            // vertex shader
            vertex = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertex, 1, &vShaderCode, NULL);
            glCompileShader(vertex);
            checkCompileErrors(vertex, "VERTEX");
            // fragment Shader
            fragment = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragment, 1, &fShaderCode, NULL);
            glCompileShader(fragment);
            checkCompileErrors(fragment, "FRAGMENT");
            // if geometry shader is given, compile geometry shader
            unsigned int geometry;
            if (geometryPath != nullptr)
            {
                const char *gShaderCode = geometryCode.c_str();
                geometry = glCreateShader(GL_GEOMETRY_SHADER);
                glShaderSource(geometry, 1, &gShaderCode, NULL);
                glCompileShader(geometry);
                checkCompileErrors(geometry, "GEOMETRY");
            }
            // shader Program
            ID = glCreateProgram();
            glAttachShader(ID, vertex);
            glAttachShader(ID, fragment);
            if (geometryPath != nullptr)
                glAttachShader(ID, geometry);
            glLinkProgram(ID);
            checkCompileErrors(ID, "PROGRAM");
            // delete the shaders as they're linked into our program now and no longer necessary
            glDeleteShader(vertex);
            glDeleteShader(fragment);
            if (geometryPath != nullptr)
                glDeleteShader(geometry);
        }
        // activate the shader
        // ------------------------------------------------------------------------
        void use() const
        {
            glUseProgram(ID);
        }
        // deactivate the shader
        void unuse() const
        {
            glUseProgram(0);
        }
        // utility uniform functions
        // ------------------------------------------------------------------------
        void setBool(const std::string &name, bool value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
        }
        // ------------------------------------------------------------------------
        void setInt(const std::string &name, int value) const
        {
            glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
        }
        // ------------------------------------------------------------------------
        void setFloat(const std::string &name, float value) const
        {
            glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
        }
        // ------------------------------------------------------------------------
        void setVec2(const std::string &name, const glm::vec2 &value) const
        {
            glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec2(const std::string &name, float x, float y) const
        {
            glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
        }
        // ------------------------------------------------------------------------
        void setVec3(const std::string &name, const glm::vec3 &value) const
        {
            glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec3(const std::string &name, float x, float y, float z) const
        {
            glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
        }
        // ------------------------------------------------------------------------
        void setVec4(const std::string &name, const glm::vec4 &value) const
        {
            glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
        }
        void setVec4(const std::string &name, float x, float y, float z, float w) const
        {
            glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
        }
        // ------------------------------------------------------------------------
        void setMat2(const std::string &name, const glm::mat2 &mat) const
        {
            glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat3(const std::string &name, const glm::mat3 &mat) const
        {
            glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }
        // ------------------------------------------------------------------------
        void setMat4(const std::string &name, const glm::mat4 &mat) const
        {
            glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
        }

    private:
        // utility function for checking shader compilation/linking errors.
        // ------------------------------------------------------------------------
        void checkCompileErrors(GLuint shader, std::string type)
        {
            GLint success;
            GLchar infoLog[1024];
            if (type != "PROGRAM")
            {
                glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
                if (!success)
                {
                    glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "Shader " << this->m_name << " ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                              << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                }
            }
            else
            {
                glGetProgramiv(shader, GL_LINK_STATUS, &success);
                if (!success)
                {
                    glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                    std::cout << "Shader " << this->m_name << " ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                              << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
                }
            }
        }
    };

    inline size_t Typesize(GLenum type)
    {
        size_t size = 0;
#define CASE(Enum, Count, Type)      \
    case Enum:                       \
        size = sizeof(Type) * Count; \
        break;
        switch (type)
        {
            CASE(GL_FLOAT, 1, GLfloat);
            CASE(GL_FLOAT_VEC2, 2, GLfloat);
            CASE(GL_FLOAT_VEC3, 3, GLfloat);
            CASE(GL_FLOAT_VEC4, 4, GLfloat);
            CASE(GL_DOUBLE, 1, GLdouble);
            CASE(GL_DOUBLE_VEC2, 2, GLdouble);
            CASE(GL_DOUBLE_VEC3, 3, GLdouble);
            CASE(GL_DOUBLE_VEC4, 4, GLdouble);
            CASE(GL_INT, 1, GLint);
            CASE(GL_INT_VEC2, 2, GLint);
            CASE(GL_INT_VEC3, 3, GLint);
            CASE(GL_INT_VEC4, 4, GLint);
            CASE(GL_UNSIGNED_INT, 1, GLuint);
            CASE(GL_UNSIGNED_INT_VEC2, 2, GLuint);
            CASE(GL_UNSIGNED_INT_VEC3, 3, GLuint);
            CASE(GL_UNSIGNED_INT_VEC4, 4, GLuint);
            CASE(GL_BOOL, 1, GLboolean);
            CASE(GL_BOOL_VEC2, 2, GLboolean);
            CASE(GL_BOOL_VEC3, 3, GLboolean);
            CASE(GL_BOOL_VEC4, 4, GLboolean);
            CASE(GL_FLOAT_MAT2, 4, GLfloat);
            CASE(GL_FLOAT_MAT3, 9, GLfloat);
            CASE(GL_FLOAT_MAT4, 16, GLfloat);
            CASE(GL_FLOAT_MAT2x3, 6, GLfloat);
            CASE(GL_FLOAT_MAT2x4, 8, GLfloat);
            CASE(GL_FLOAT_MAT3x2, 6, GLfloat);
            CASE(GL_FLOAT_MAT3x4, 12, GLfloat);
            CASE(GL_FLOAT_MAT4x2, 8, GLfloat);
            CASE(GL_FLOAT_MAT4x3, 12, GLfloat);
            CASE(GL_DOUBLE_MAT2, 4, GLdouble);
            CASE(GL_DOUBLE_MAT3, 9, GLdouble);
            CASE(GL_DOUBLE_MAT4, 16, GLdouble);
#undef CASE
        default:
            assert(false && "Unknown type");
            break;
        }
        return size;
    }
} // namespace Renderer
#endif