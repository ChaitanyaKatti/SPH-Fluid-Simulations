#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 lookAt;
    glm::vec3 worldUp;
    
    glm::vec3 front;
    glm::vec3 right;
    glm::vec3 up;
    
    // float aspectRatio;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float fov;

    Camera(glm::vec3 position, glm::vec3 lookat, glm::vec3 worldUp);
    
    void ProcessInput(GLFWwindow *window, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessMouseScroll(float yoffset);
    
    glm::mat4 GetViewMatrix();
    glm::mat4 GetProjectionMatrix();
    glm::mat4 GetViewProjectionMatrix();

private:
    void updateCameraVectors();
};
