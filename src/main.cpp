// #define IMGUI

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <utils.hpp>
#include <shader.hpp>
#include <texture.hpp>
#include <mesh.hpp>
#include <camera.hpp>

int main()
{
    GLFWwindow *window = initWindow();
    if (window == NULL)
    {
        return -1;
    }
#ifdef IMGUI
    imguiInit(window);
#endif

    // Build and compile our shader program
    Shader defaultShader(ASSETS_PATH "shaders/default.vs", ASSETS_PATH "shaders/default.fs");
    defaultShader.use();

    // Texture
    Texture texture0(0, ASSETS_PATH "images/test.png", DIFFUSE);
    texture0.Bind();
    defaultShader.setInt("texture0", 0);

    Mesh ball = Mesh(ASSETS_PATH "models/cube.obj", &texture0, &defaultShader);

    Camera camera = Camera(glm::vec3(5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), SCR_WIDTH / SCR_HEIGHT);

    // OpenGL state
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
#ifdef IMGUI
        imguiNewFrame();
#endif
        camera.ProcessKeyboard(window);
        processInput(window);

        defaultShader.setMat4("model", glm::mat4(1.0f));
        defaultShader.setMat4("viewProj",
                              camera.GetViewProjectionMatrix());

        // Render
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);
        // draw our first triangle
        defaultShader.use();
        ball.Draw();

        // Swap buffers and poll IO events
#ifdef IMGUI
        imguiRender();
#endif
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    defaultShader.~Shader();
    texture0.~Texture();
#ifdef IMGUI
    imguiDestroy();
#endif
    glfwTerminate();
    return 0;
}