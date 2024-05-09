// #define IMGUI
#define NUM_INS_DIM 100
#define NUM_INS NUM_INS_DIM * NUM_INS_DIM * NUM_INS_DIM

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

    // Texture
    Texture texture0(GL_TEXTURE0, ASSETS_PATH "images/test.png", DIFFUSE);
    textureShader.setInt("texture0", 0);

    // Mesh
    Mesh ball0 = Mesh(ASSETS_PATH "models/icosphere1.obj", &texture0, &textureShader);
    Mesh ball1 = Mesh(ASSETS_PATH "models/icosphere2.obj", &texture0, &textureShader);
    Mesh ball2 = Mesh(ASSETS_PATH "models/icosphere3.obj", &texture0, &textureShader);
    Mesh cube = Mesh(ASSETS_PATH "models/cube.obj", nullptr, &colorShader);

    // Camera
    Camera camera = Camera(glm::vec3(10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Instances for LODs
    const double lod0_fraction = 0.001;
    const double lod1_fraction = 0.01;
    glm::vec3 *offsets = getUniformVec3Array(NUM_INS_DIM, 10.0f);
    // Sort offsets by distance to camera
    std::sort(offsets, offsets + NUM_INS, [&camera](glm::vec3 a, glm::vec3 b)
        { return glm::length(a - camera.position) < glm::length(b - camera.position); });
    ball0.SetInstances((int)(NUM_INS * lod0_fraction), offsets);
    ball1.SetInstances(NUM_INS - (int)(NUM_INS * lod0_fraction), offsets + (int)(NUM_INS * lod0_fraction));
    ball2.SetInstances(NUM_INS - (int)(NUM_INS * lod1_fraction), offsets + (int)(NUM_INS * lod1_fraction));
    free(offsets);

    // OpenGL state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    auto lastFrame = Clock::now();

    // glm::vec3 lastCameraPos = camera.position;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        // Start the Dear ImGui frame
#ifdef IMGUI
        imguiNewFrame();
#endif
        camera.ProcessKeyboard(window, deltaTime);
        processInput(window);

        // Dynamic LOD
        // if(glm::length(camera.position - lastCameraPos) > 0.1f)
        // {
        //     auto sortStartTime = Clock::now();
        //     lastCameraPos = camera.position;
        //     std::sort(offsets, offsets + NUM_INS, [&camera](glm::vec3 a, glm::vec3 b)
        //             { return glm::length(a - camera.position) < glm::length(b - camera.position); });
        //     auto sortEndTime = Clock::now();
        //     std::cout << "Sort time: " << std::chrono::duration<float, std::chrono::milliseconds::period>(sortEndTime - sortStartTime).count() << "ms" << std::endl;
        //     ball0.SetInstances((int)(NUM_INS * lod0_fraction), offsets);
        //     ball1.SetInstances(NUM_INS - (int)(NUM_INS * lod0_fraction), offsets + (int)(NUM_INS * lod0_fraction));
        //     ball2.SetInstances(NUM_INS - (int)(NUM_INS * lod1_fraction), offsets + (int)(NUM_INS * lod1_fraction));
        // }

        // Set the view and projection matrix in the shader
        textureShader.setMat4("viewProjMatrix",
                              camera.GetViewProjectionMatrix());
        textureShader.setVec3("viewPos",
                              camera.position);
        colorShader.setMat4("viewProjMatrix",
                            camera.GetViewProjectionMatrix());

        // Render
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        ball0.DrawInstances(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)));
        ball1.DrawInstances(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.02f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)));
        ball2.DrawInstances(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(0.04f)), (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f)));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(2.0f);
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
        std::cout << "\x1B[2J\x1B[H" << "FPS: " << FPS << std::endl;
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