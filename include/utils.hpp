#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// // settings
// unsigned int SCR_WIDTH;
// unsigned int SCR_HEIGHT;

glm::vec3 getRandVec3();

void genRandVec3Array(glm::vec3 *arr, int n, float scale);

void genUniformVec3Array(glm::vec3 *arr, int n, float scale);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void processInput(GLFWwindow *window);

GLFWwindow *initWindow();

#ifdef IMGUI
void imguiInit(GLFWwindow *window);
void imguiNewFrame();
void imguiRender();
void imguiDestroy();
#endif