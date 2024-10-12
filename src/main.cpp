#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>
#include <imgui.h>

#include <shader.hpp>
#include <texture.hpp>
#include <mesh.hpp>
#include <particles.hpp>
#include <spatialgrid.hpp>
#include <camera.hpp>
#include <utils.hpp>
#include <gui.hpp>
#include <config.hpp>
#include <omp.h>

typedef std::chrono::high_resolution_clock Clock;

double deltaTime = 0.0f;

int main()
{
    omp_set_num_threads(8);
    std::cout << "Hello!" << std::endl;
    GLFWwindow *window = initWindow();
    if (window == NULL)
    {
        return -1;
    }

    imguiInit(window);
    ImGuiIO &io = ImGui::GetIO();

    // Shaders
    Shader colorShader(ASSETS_PATH "shaders/color/color.vs",
                       ASSETS_PATH "shaders/color/color.fs");
    Shader pointSphereShader(ASSETS_PATH "shaders/geometryPoint/pointSphere.vs",
                             ASSETS_PATH "shaders/geometryPoint/pointSphere.fs",
                             ASSETS_PATH "shaders/geometryPoint/pointSphere.gs");

    // Camera
    Camera camera = Camera(glm::vec3(18.0f, 8.0f, 5.0f), glm::vec3(0.0f, 4.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // Grid and Particles
    SpatialGrid grid(glm::vec3(10.0f), glm::mat4(10.0f), &colorShader);
    Particles particles(MASS, DENSITY, 0.1f, NUM_INS, &grid, &pointSphereShader);

    // OpenGL state
    configureOpenGL();

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
        // grid.setTransform(glm::rotate(grid.getTransform(), 0.5f*dt, glm::vec3(0.0f, 1.0f, 0.0f)));
        particles.update(dt); // Update particles

        // Set the view and projection matrix in the shader
        colorShader.setMat4("viewProjMatrix", camera.GetViewProjectionMatrix());
        colorShader.setVec4("color", glm::vec4(1.0f, 0.0f, 1.0f, 0.8f));

        pointSphereShader.setMat4("viewMatrix", camera.GetViewMatrix());
        pointSphereShader.setMat4("projMatrix", camera.GetProjectionMatrix());
        pointSphereShader.setVec3("eyePos", camera.position);
        pointSphereShader.setFloat("uTime", glfwGetTime());

        // Render
        glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Draw Particles
        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        particles.Draw();

        // Draw Cube
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        grid.drawBoundary();

        // Swap buffers and poll IO events
        imguiRender();
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculate FPS
        auto currentFrame = Clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrame - lastFrame).count();
        lastFrame = currentFrame;
    }

    // Cleanup
    imguiDestroy();
    glfwTerminate();

    return 0;
}