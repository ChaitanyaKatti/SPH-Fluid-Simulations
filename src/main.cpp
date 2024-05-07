// #define IMGUI

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

#include <utils.hpp>
#include <shader.hpp>
#include <texture.hpp>
#include <mesh.hpp>
#include <camera.hpp>

typedef std::chrono::high_resolution_clock Clock;
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

double FPS = 60.0f;
double deltaTime = 0.0f;

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
    Shader textureShader(ASSETS_PATH "shaders/texture_instanced.vs", ASSETS_PATH "shaders/texture.fs");
    Shader colorShader(ASSETS_PATH "shaders/color.vs", ASSETS_PATH "shaders/color.fs");

    // Texture
    Texture texture0(GL_TEXTURE0, ASSETS_PATH "images/test.png", DIFFUSE);
    textureShader.setInt("texture0", 0);

    Mesh ball = Mesh(ASSETS_PATH "models/icosphere.obj", &texture0, &textureShader);
    glm::vec3 *offsets = getUniformVec3Array(50, 10.0f);
    ball.SetInstances(50 * 50 * 50, offsets);
    free(offsets);
    // ball.SetInstances(1000, getRandVec3Array(1000, 10.0f));
    Mesh cube = Mesh(ASSETS_PATH "models/cube.obj", nullptr, &colorShader);

    Camera camera = Camera(glm::vec3(10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    auto lastFrame = Clock::now();
    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
#ifdef IMGUI
        imguiNewFrame();
#endif
        camera.ProcessKeyboard(window, deltaTime);
        processInput(window);

        // Set the view and projection matrix in the shader
        textureShader.setMat4("viewProjMatrix",
                              camera.GetViewProjectionMatrix());
        colorShader.setMat4("viewProjMatrix",
                            camera.GetViewProjectionMatrix());

        // Render
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        ball.DrawInstanced(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)), 50 * 50 * 50);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
        cube.Draw(glm::scale(glm::translate(glm::mat4(1), glm::vec3(5)), glm::vec3(5)));

        // Swap buffers and poll IO events
#ifdef IMGUI
        imguiRender();
#endif
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Measure time
        auto currentFrame = Clock::now();
        deltaTime = 0.99*deltaTime + 0.01*std::chrono::duration<float, std::chrono::seconds::period>(currentFrame - lastFrame).count();
        lastFrame = currentFrame;
        FPS = 0.9*FPS + 0.1/(deltaTime + 0.0001);
        std::cout << "FPS: " << int(FPS) << std::endl;
    }

    // Cleanup
    textureShader.~Shader();
    colorShader.~Shader();
    texture0.~Texture();
#ifdef IMGUI
    imguiDestroy();
#endif
    glfwTerminate();
    return 0;
}