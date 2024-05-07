#include <camera.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


Camera::Camera(glm::vec3 position, glm::vec3 lookAt, glm::vec3 worldUp, float aspectRatio)
{
    this->position = position;
    this->worldUp = glm::normalize(worldUp);

    this->front = glm::normalize(lookAt - position);
    this->lookAt = position + front;
    this->right = glm::normalize(glm::cross(this->lookAt - this->position, this->worldUp));
    this->up = glm::normalize(glm::cross(right, front));

    this->aspectRatio = aspectRatio;
    this->Yaw = glm::degrees(atan2(front.z, front.x));
    this->Pitch = glm::degrees(asin(front.y));
    this->MovementSpeed = 0.01f;
    this->MouseSensitivity = 0.1f;
    this->fov = 75.0f;

    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
    return glm::lookAt(position, lookAt, worldUp);
}

glm::mat4 Camera::GetProjectionMatrix()
{
    return glm::perspective(glm::radians(this->fov), aspectRatio, 0.1f, 100.0f);
}

glm::mat4 Camera::GetViewProjectionMatrix()
{
    return GetProjectionMatrix() * GetViewMatrix();
}

void Camera::ProcessKeyboard(GLFWwindow *window)
{   
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        this->position += MovementSpeed * this->front;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        this->position -= MovementSpeed * this->front;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        this->position -= MovementSpeed * this->right;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        this->position += MovementSpeed * this->right;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        this->position += MovementSpeed * this->worldUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        this->position -= MovementSpeed * this->worldUp;
    
    this->lookAt = this->position + this->front;


    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        this->Pitch += 0.05f;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        this->Pitch -= 0.05f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        this->Yaw -= 0.05f;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        this->Yaw += 0.05f;
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
