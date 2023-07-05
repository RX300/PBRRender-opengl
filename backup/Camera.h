#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include "Input.h"

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement
{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 4.5f;
const float SENSITIVITY = 0.025f;
const float ZOOM = 45.0f;

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }
    // constructor with scalar values
    Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
    {
        Position = glm::vec3(posX, posY, posZ);
        WorldUp = glm::vec3(upX, upY, upZ);
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)
            Position += Front * velocity;
        if (direction == BACKWARD)
            Position -= Front * velocity;
        if (direction == LEFT)
            Position -= Right * velocity;
        if (direction == RIGHT)
            Position += Right * velocity;
        if (direction == UP)
            Position += Up * velocity;
        if (direction == DOWN)
            Position -= Up * velocity;
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {

        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.

    void ProcessMouseMovement(GLboolean constrainPitch = true)
    {
        if (Renderer::Input::GetInstance().MouseMoved())
        {
            const auto xPos = Renderer::Input::GetInstance().GetMouseX();
            const auto yPos = Renderer::Input::GetInstance().GetMouseY();

            if (m_firstMouse)
            {
                m_prevX = xPos;
                m_prevY = yPos;
                m_firstMouse = false;
            }

            float xoffset = xPos - m_prevX;
            float yoffset = m_prevY - yPos; // reversed since y-coordinates go from bottom to top

            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity;

            Yaw += xoffset;
            Pitch += yoffset;

            // make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrainPitch)
            {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            // update Front, Right and Up Vectors using the updated Euler angles
            updateCameraVectors();
        }
    }
    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset)
    {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)
            Zoom = 1.0f;
        if (Zoom > 45.0f)
            Zoom = 45.0f;
    }
    void IncreaseMovementSpeed(float value)
    {
        MovementSpeed += value;
    }

    void DecreaseMovementSpeed(float value)
    {
        if (MovementSpeed - value > 0)
            MovementSpeed -= value;
    }

    void IncreaseMouseSensitivity(float value)
    {
        MouseSensitivity += value;
    }

    void DecreaseMouseSensitivity(float value)
    {
        if (MouseSensitivity - value > 0)
            MouseSensitivity -= value;
    }

    void update(const double deltaTime)
    {
        if (Renderer::Input::GetInstance().IsKeyPressed(GLFW_KEY_TAB))
        {
            m_dirty = !m_dirty;
        }
        if (m_dirty)
        {
            // Update Mouse
            ProcessMouseMovement();
            // Update Keyboard
            // 控制相机移动
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_W))
            {
                ProcessKeyboard(Camera_Movement::FORWARD, deltaTime);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_S))
            {
                ProcessKeyboard(Camera_Movement::BACKWARD, deltaTime);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_A))
            {
                ProcessKeyboard(Camera_Movement::LEFT, deltaTime);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_D))
            {
                ProcessKeyboard(Camera_Movement::RIGHT, deltaTime);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_Q))
            {
                ProcessKeyboard(Camera_Movement::UP, deltaTime);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_E))
            {
                ProcessKeyboard(Camera_Movement::DOWN, deltaTime);
            }
            // 调整摄像机设置
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_UP))
            {
                IncreaseMovementSpeed(0.05f);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_DOWN))
            {
                DecreaseMovementSpeed(0.05f);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_PAGE_UP))
            {
                IncreaseMouseSensitivity(0.05f);
            }
            if (Renderer::Input::GetInstance().IsKeyHeld(GLFW_KEY_PAGE_DOWN))
            {
                DecreaseMouseSensitivity(0.05f);
            }
        }
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        // also re-calculate the Right and Up vector
        Right = glm::normalize(glm::cross(Front, WorldUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        Up = glm::normalize(glm::cross(Right, Front));
    }

    // Mouse positions
    bool m_firstMouse{true};
    double m_prevX{0.0}, m_prevY{0.0};
    // Should we update the camera attributes?
    bool m_dirty{true};
};
#endif