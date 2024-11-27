#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <shader.hpp>

class Particles {
public:
    Particles(Shader* shader);
    ~Particles();
    
    void update();
    void Draw();
    void reset();

private:
    void setupVAO();
    void updateGPUBuffers();
    void initCUDA();
    void cleanupCUDA();

    Shader* shader;
    unsigned int VAO, VBO;
    
    std::vector<float> h_positions;     // Host (CPU) positions
    std::vector<float> h_velocities;    // Host velocities
    std::vector<float> h_accelerations; // Host accelerations
    
    float* d_positions = nullptr;     // Device (GPU) positions
    float* d_velocities = nullptr;    // Device velocities
    float* d_accelerations = nullptr; // Device accelerations
    
    static const int MAX_PARTICLES = 10000;
};
