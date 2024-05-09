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


glm::vec3 getRandVec3(){
    return glm::vec3(
        static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
        static_cast <float> (rand()) / static_cast <float> (RAND_MAX),
        static_cast <float> (rand()) / static_cast <float> (RAND_MAX)
    );
}

glm::vec3* getRandVec3Array(int n, float scale=1.0f){
    glm::vec3* arr = new glm::vec3[n];
    for(int i=0; i<n; i++){
        arr[i] = getRandVec3() * scale;
    }
    return arr;
}

glm::vec3* getUniformVec3Array(int n, float scale=1.0f){
    if(n <= 0){
        return nullptr;
    }
    glm::vec3* arr = new glm::vec3[n*n*n];
    for(int i=0; i<n; i++){
        for(int j=0; j<n; j++){
            for(int k=0; k<n; k++){
                arr[i*n*n + j*n + k] = glm::vec3(i, j, k) *(scale / (n-1));
            }
        }
    }
    std::cout << "Uniform array created of size: " << (float)sizeof(glm::vec3) * n*n*n /1024/1024<< " Mbs" << std::endl;
    return arr;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
#ifdef IMGUI
    else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS){
        ImGui::SetNextWindowCollapsed(true);
    }
    else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE){
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
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "More Triangles", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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
void imguiInit(GLFWwindow* window){
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);          // Second param install_callback=true will install GLFW callbacks and chain to existing ones.
    ImGui_ImplOpenGL3_Init();
}

void imguiNewFrame(){
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();
}

void imguiRender(){
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void imguiDestroy(){
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
#endif