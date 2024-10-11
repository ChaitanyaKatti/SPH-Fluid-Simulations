#pragma once

#include <GLFW/glfw3.h>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

GLFWwindow *initWindow();

void imguiInit(GLFWwindow *window);
void imguiNewFrame();
void imguiRender();
void imguiDestroy();