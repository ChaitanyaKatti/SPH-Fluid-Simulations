#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>
#include <imgui.h>

#include <shader.hpp>
#include <particle_system.hpp>
#include <camera.hpp>
#include <gui.hpp>
#include <config.hpp>

typedef std::chrono::high_resolution_clock Clock;

double deltaTime = 0.0f;

int main()
{
    std::cout << "Hello!" << std::endl;
    GLFWwindow *window = initWindow();
    if (window == NULL)
    {
        return -1;
    }

    imguiInit(window);
    ImGuiIO &io = ImGui::GetIO();

    // Shaders
    Shader pointSphereShader(ASSETS_PATH "shaders/pointSphere/pointSphere.vs",
                             ASSETS_PATH "shaders/pointSphere/pointSphere.fs",
                             ASSETS_PATH "shaders/pointSphere/pointSphere.gs");

    // Camera
    Camera camera = Camera(glm::vec3(5.0f, 5.0f, 10.0f), glm::vec3(5.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Meshes and Particles
    ParticleSystem particles(&pointSphereShader);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE);
    auto lastFrame = Clock::now();

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
        imguiNewFrame();

        // Input processing
        if (!io.WantCaptureKeyboard)
        {
            camera.ProcessInput(window, deltaTime); // Camera input
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            { // Exit program
                glfwSetWindowShouldClose(window, true);
            }
            if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
            { // Reset particles
                particles.resetParticles();
            }
        }

        // Update particles using CUDA
        particles.updateParticles(); // Call the CUDA-based update method

        pointSphereShader.setCamera(camera);

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        // renderParticles Particles
        particles.renderParticles();

        // Swap buffers and poll IO events
        imguiRender();
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculate FPS
        auto currentFrame = Clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrame - lastFrame).count();
        lastFrame = currentFrame;
    }

    // Cleanup other resources
    imguiDestroy();
    glfwTerminate();

    return 0;
}
