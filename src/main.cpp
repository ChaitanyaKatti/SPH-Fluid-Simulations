// #define IMGUI
#define NUM_INS_DIM 10
#define NUM_INS NUM_INS_DIM *NUM_INS_DIM *NUM_INS_DIM

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>
#include <chrono>

#include <shader.hpp>
#include <texture.hpp>
#include <mesh.hpp>
#include <particles.hpp>
#include <camera.hpp>
#include <utils.hpp>

typedef std::chrono::high_resolution_clock Clock;
extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

double FPS = 60.0f;
double deltaTime = 0.0f;


int main()
{
    std::cout << "Hello!" << std::endl;
    GLFWwindow *window = initWindow();
    if (window == NULL)
    {
        return -1;
    }
#ifdef IMGUI
    imguiInit(window);
#endif

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
    // genRandVec3Array(positions, NUM_INS, 10.0f);
    // genRandVec3Array(colors, NUM_INS, 1.0f);
    genUniformVec3Array(positions, NUM_INS_DIM , 7.0f);
    genUniformVec3Array(colors, NUM_INS_DIM , 1.0f);
    // Sort positions by distance to camera
    // std::sort(positions, positions + NUM_INS, [&camera](glm::vec3 a, glm::vec3 b)
    //           { return glm::length(a - camera.position) > glm::length(b - camera.position); });
    // std::sort(colors, colors + NUM_INS, [&camera](glm::vec3 a, glm::vec3 b)
    //           { return glm::length(a - camera.position) > glm::length(b - camera.position); });
    // Meshes and Particles
    Mesh cube = Mesh(ASSETS_PATH "models/cube.obj", nullptr, &colorShader);
    Particles particles(1.0f, NUM_INS/(7*7*7), 0.1f, positions, colors, NUM_INS, &pointSphereShader);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    // glEnable(GL_CULL_FACE);
    // glCullFace(GL_BACK);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glPointSize(1.0f);
    glLineWidth(2.0f);
    glEnable(GL_MULTISAMPLE);
    auto lastFrame = Clock::now();

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
#ifdef IMGUI
        imguiNewFrame();
#endif
        if(glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
            genUniformVec3Array(positions, NUM_INS_DIM , 7.0f);
            particles.setPositions(positions);
        }
        // if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS){
        // }
            particles.update();

        // Input
        camera.ProcessKeyboard(window, deltaTime);
        processInput(window);

        // Set the view and projection matrix in the shader
        colorShader.setMat4("viewProjMatrix", camera.GetViewProjectionMatrix());
        colorShader.setVec3("color", glm::vec3(1.0f, 0.0f, 1.0f));

        pointSphereShader.setMat4("viewMatrix", camera.GetViewMatrix());
        pointSphereShader.setMat4("projMatrix", camera.GetProjectionMatrix());
        pointSphereShader.setVec3("eyePos", camera.position);
        pointSphereShader.setFloat("uTime", glfwGetTime());

        // Render
        glClearColor(0.9f, 0.9f, 0.8f, 0.0f);
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
#ifdef IMGUI
        imguiRender();
#endif
        glfwSwapBuffers(window);
        glfwPollEvents();

        // Calculate FPS
        auto currentFrame = Clock::now();
        deltaTime = 0.9 * deltaTime + 0.1 * std::chrono::duration<float, std::chrono::seconds::period>(currentFrame - lastFrame).count();
        lastFrame = currentFrame;
        FPS = 0.9 * FPS + 0.1 / (deltaTime + 0.0001);
        // Show FPS
        std::cout << std::fixed;
        std::cout.precision(1);
        std::cout << "\x1b[1;31m" << "FPS: " << FPS << "\r\x1b[0m";
    }

    // Cleanup
    delete[] positions;
    delete[] colors;
#ifdef IMGUI
    imguiDestroy();
#endif
    glfwTerminate();
    return 0;
}