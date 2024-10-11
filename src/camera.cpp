#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <camera.hpp>
#include <config.hpp>

Camera::Camera(glm::vec3 position, glm::vec3 lookAt, glm::vec3 worldUp)
{
    this->position = position;
    this->worldUp = glm::normalize(worldUp);

    this->front = glm::normalize(lookAt - position);
    this->lookAt = position + front;
    this->right = glm::normalize(glm::cross(this->lookAt - this->position, this->worldUp));
    this->up = glm::normalize(glm::cross(right, front));

    this->Yaw = glm::degrees(atan2(front.z, front.x));
    this->Pitch = glm::degrees(asin(front.y));
    this->MovementSpeed = 5.0f;
    this->MouseSensitivity = 40.0f;
    this->fov = 75.0f;

    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(position, lookAt, worldUp);
}

glm::mat4 Camera::GetProjectionMatrix()
{
    return glm::perspective(glm::radians(this->fov), (float)SCR_WIDTH / (SCR_HEIGHT), 1.0f, 40.0f);
}

glm::mat4 Camera::GetViewProjectionMatrix()
{
    return GetProjectionMatrix() * GetViewMatrix();
}

void Camera::ProcessInput(GLFWwindow *window, float deltaTime)
{
    // if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
    //     this->position = glm::vec3(10.0f);
    //     this->Yaw = -135.0f;
    //     this->Pitch = -35.3f;
    // }
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        this->position += deltaTime * MovementSpeed * this->front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        this->position -= deltaTime * MovementSpeed * this->front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        this->position -= deltaTime * MovementSpeed * this->right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        this->position += deltaTime * MovementSpeed * this->right;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        this->position += deltaTime * MovementSpeed * this->worldUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        this->position -= deltaTime * MovementSpeed * this->worldUp;

    this->lookAt = this->position + this->front;

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        this->Pitch += deltaTime * MouseSensitivity;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        this->Pitch -= deltaTime * MouseSensitivity;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        this->Yaw -= deltaTime * MouseSensitivity;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        this->Yaw += deltaTime * MouseSensitivity;
    this->Pitch = glm::clamp(this->Pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= this->MouseSensitivity;
    yoffset *= this->MouseSensitivity;

    this->Yaw += xoffset;
    this->Pitch += yoffset;
    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 45.0f)
        fov = 45.0f;
}

void Camera::updateCameraVectors()
{
    this->front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    this->front.y = sin(glm::radians(Pitch));
    this->front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    this->front = glm::normalize(front);
    this->right = glm::normalize(glm::cross(front, worldUp));
    this->up = glm::normalize(glm::cross(right, front));
}
