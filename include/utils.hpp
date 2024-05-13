#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#ifdef IMGUI
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#endif
#include <iostream>
#include <fstream>
#include <string>

// settings
unsigned int SCR_WIDTH = 1400;
unsigned int SCR_HEIGHT = 800;
float vectorFieldScale = 0.005f;

glm::vec3 getRandVec3()
{
    return glm::vec3(
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
}

void genRandVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    for (int i = 0; i < n; i++)
    {
        arr[i] = getRandVec3() * scale;
    }
}

inline glm::vec3 vectorField(glm::vec3 p)
{
    // Lorenz attractor
    return glm::vec3(10.0f * (p.z - p.x),
                     (p.x * p.z - 8.0f / 3.0f * p.y),
                     (p.x * (28.0f - p.y) - p.z));
}

void applyVectorField(glm::vec3 *positions, glm::vec3 *colors, int n)
{
#pragma omp parallel for
    for (int i = 0; i < n; i++)
    {
        glm::vec3 vel = vectorField(positions[i]);
        positions[i] += vel * vectorFieldScale;
        float speed = glm::length(vel) / 50.0f;
        colors[i] = glm::vec3(speed, 0.0, 1.0 - speed);
    }
}

void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    if (n <= 0)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                arr[i * n * n + j * n + k] = glm::vec3(i, j, k) * (scale / (n - 1)) + getRandVec3() * 0.01f;
            }
        }
    }
    std::cout << "Uniform array created of size: " << (float)sizeof(glm::vec3) * n * n * n / 1024 / 1024 << " Mbs" << std::endl;
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        vectorFieldScale *= 1.1f;
        vectorFieldScale = std::min(vectorFieldScale, 0.01f);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        vectorFieldScale *= 0.9f;
        vectorFieldScale = std::max(vectorFieldScale, 0.0001f);
    }
#ifdef IMGUI
    else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
    {
        ImGui::SetNextWindowCollapsed(true);
    }
    else if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE)
    {
        ImGui::SetNextWindowCollapsed(false);
    }
#endif
}

GLFWwindow *initWindow()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // glfwWindowHint(GLFW_DECORATED, GL_FALSE);
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "SPH Simulation", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(0); // Enable vsync
    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return nullptr;
    }
    // Set window position to center
    const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
    int posX = (mode->width - SCR_WIDTH) / 2;
    int posY = (mode->height - SCR_HEIGHT) / 2;
    glfwSetWindowPos(window, posX, posY);
    return window;
}

#ifdef IMGUI
void imguiInit(GLFWwindow *window)
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true); // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

void imguiNewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
}

void imguiRender()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void imguiDestroy()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif