// #define IMGUI
#define NUM_INS_DIM 100
#define NUM_INS NUM_INS_DIM *NUM_INS_DIM *NUM_INS_DIM

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

    // Shaders
    Shader textureShader(ASSETS_PATH "shaders/texture_instanced.vs", ASSETS_PATH "shaders/texture.fs");
    Shader colorShader(ASSETS_PATH "shaders/color.vs", ASSETS_PATH "shaders/color.fs");
    Shader pointSpriteShader(ASSETS_PATH "shaders/pointSprite.vs", ASSETS_PATH "shaders/pointSprite.fs");
    Shader pointSphereShader(ASSETS_PATH "shaders/pointSphere.vs", ASSETS_PATH "shaders/pointSphere.fs");
    
    // Texture
    Texture texture0(GL_TEXTURE0, ASSETS_PATH "images/earth.jpg", DIFFUSE);
    textureShader.setInt("texture0", 0);
    pointSpriteShader.setInt("texture0", 0);

    // Camera
    Camera camera = Camera(glm::vec3(12.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Instances for LODs
    glm::vec3 *offsets = getUniformVec3Array(NUM_INS_DIM, 10.0f);
    // Sort offsets by distance to camera
    // std::sort(offsets, offsets + NUM_INS, [&camera](glm::vec3 a, glm::vec3 b)
    //           { return glm::length(a - camera.position) > glm::length(b - camera.position); });
    
    // Meshes and Points
    Mesh cube = Mesh(ASSETS_PATH "models/cube.obj", nullptr, &colorShader);
    Points points(offsets, NUM_INS, &pointSphereShader);
    free(offsets);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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
        camera.ProcessKeyboard(window, deltaTime);
        processInput(window);

        // Set the view and projection matrix in the shader
        textureShader.setMat4("viewProjMatrix", camera.GetViewProjectionMatrix());
        textureShader.setVec3("viewPos", camera.position);
        colorShader.setMat4("viewProjMatrix", camera.GetViewProjectionMatrix());
        colorShader.setVec3("color", glm::vec3(1.0f, 0.0f, 1.0f));
        pointSpriteShader.setMat4("viewMatrix", camera.GetViewMatrix());
        pointSpriteShader.setMat4("projMatrix", camera.GetProjectionMatrix());
        pointSpriteShader.setVec3("eyePos", camera.position);
        pointSpriteShader.setFloat("uTime", glfwGetTime());
        pointSphereShader.setMat4("viewMatrix", camera.GetViewMatrix());
        pointSphereShader.setMat4("projMatrix", camera.GetProjectionMatrix());
        pointSphereShader.setVec3("eyePos", camera.position);
        pointSphereShader.setFloat("uTime", glfwGetTime());

        // Render
        glClearColor(0.1f, 0.1f, 0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        // glEnable(GL_CULL_FACE);
        // glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Draw Cube
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDisable(GL_CULL_FACE);
        cube.Draw(glm::scale(glm::translate(glm::mat4(1), glm::vec3(5)), glm::vec3(5)));

        // Draw Points
        points.Draw();


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
        std::cout << "\x1B[2J\x1B[H" << "FPS: " << FPS << std::endl;
    }

    // Cleanup
    textureShader.~Shader();
    colorShader.~Shader();
    pointSpriteShader.~Shader();
    texture0.~Texture();
#ifdef IMGUI
    imguiDestroy();
#endif
    glfwTerminate();
    return 0;
}