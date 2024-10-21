
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
    Camera camera = Camera(glm::vec3(16.0f, 9.0f, 9.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Instances for LODs
    glm::vec3 *positions = new glm::vec3[NUM_INS];
    glm::vec3 *colors = new glm::vec3[NUM_INS];
    genUniformVec3Array(positions, NUM_INS_DIM, 7.0f);
    genUniformVec3Array(colors, NUM_INS_DIM, 1.0f);
    // Meshes and Particles
    Mesh cube = Mesh(ASSETS_PATH "models/cube.obj", nullptr, &colorShader);
    Particles particles(MASS, NUM_INS / (8 * 8 * 8), 0.1f, positions, colors, NUM_INS, &pointSphereShader);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    // glEnable(GL_PROGRAM_POINT_SIZE);
    // glPointSize(1.0f);
    glLineWidth(2.0f);
    glEnable(GL_MULTISAMPLE);
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
                genUniformVec3Array(positions, NUM_INS_DIM, 7.0f);
                particles.setPositions(positions);
            }
        }

        particles.update(dt); // Update particles

        // Set the view and projection matrix in the shader
        colorShader.setMat4("viewProjMatrix", camera.GetViewProjectionMatrix());
        colorShader.setVec3("color", glm::vec3(1.0f, 0.0f, 1.0f));

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
        cube.Draw(glm::scale(glm::translate(glm::mat4(1), glm::vec3(5)), glm::vec3(5)));

        // Swap buffers and poll IO events
        imguiRender();
        glfwSwapBuffers(window);
        glfwPollEvents();
        std::cout << "asdasdasdasdasd" << "\n";

        // Calculate FPS
        auto currentFrame = Clock::now();
        deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentFrame - lastFrame).count();
        lastFrame = currentFrame;
    }

    // Cleanup
    delete[] positions;
    delete[] colors;
    imguiDestroy();
    glfwTerminate();

    return 0;
}