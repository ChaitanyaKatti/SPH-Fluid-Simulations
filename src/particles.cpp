#include <particles.hpp>
#include <glad/glad.h>

#define h 1.0f
#define h2 h * h
#define k 20.0f
#define mu 0.5f
#define g 9.81f
#define dt 0.02f
#define EPSILON 0.001f

inline float poly6Kernel(float r2){
    if (r2 < h2){
        return 315.0f / (64.0f * M_PI * glm::pow(h, 9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r){
    if (sqrt_r < h){
        return (float)(-45.0f / (M_PI * glm::pow(h, 6)) * glm::pow(h - sqrt_r, 2)) * glm::normalize(r);
    }
    return glm::vec3(0.0f);
}

inline float viscosityLaplacian(glm::vec3 r, float sqrt_r){
    if (sqrt_r < h){
        return 45.0f / (M_PI * glm::pow(h, 6)) * (h - sqrt_r);
    }
    return 0.0f;
}

Particles::Particles(const float mass, const float resting_density, const float radius, glm::vec3* positions, glm::vec3* colors, int num_points, Shader* const shader) : mass(mass), resting_density(resting_density), radius(radius), positions(positions), colors(colors), num_points(num_points), shader(shader) {
    setupParticles();
}

void Particles::setupParticles(){
    this->velocities = new glm::vec3[num_points];
    this->densities = new float[num_points];
    this->pressures = new float[num_points];
    this->forces = new glm::vec3[num_points];

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3) * 2, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3), num_points * sizeof(glm::vec3), colors);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)(num_points * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Particles::update(){
    calculateDensity();
    calculatePressure();
    calculateForces();
    applyForces();

    // Update VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, num_points * sizeof(glm::vec3), num_points * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Particles::Draw(){
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, num_points);
    glBindVertexArray(0);
}

void Particles::calculateDensity(){
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++){
        densities[i] = 0.0f;
        for (int j = 0; j < num_points; j++){
            if (i != j){
                glm::vec3 r = positions[i] - positions[j];
                float r2 = glm::dot(r, r);
                densities[i] += mass * poly6Kernel(r2);
            }
        }
    }
}

void Particles::calculatePressure(){
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++){
        pressures[i] = k * (densities[i] - resting_density);
    }
}

void Particles::calculateForces(){
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++){
        glm::vec3 f_pressure = glm::vec3(0.0f);
        glm::vec3 f_viscosity = glm::vec3(0.0f);
        glm::vec3 f_gravity = glm::vec3(0.0f);
        
        for (int j = 0; j < num_points; j++){
            if (i != j){
                glm::vec3 r = positions[i] - positions[j];
                float sqrt_r = glm::length(r);
                f_pressure += -mass * (pressures[i] + pressures[j]) / (2.0f * densities[j] + 0.001f) * spikyGradient(r, sqrt_r);
                f_viscosity += mu * mass * (velocities[j] - velocities[i]) / (densities[j] + 0.001f) * viscosityLaplacian(r, sqrt_r);
            }
        }
        
        f_gravity = glm::vec3(0.0f, -mass * g, 0.0f);
        forces[i] = f_pressure + f_viscosity + f_gravity;
    }
}

void Particles::applyForces(){
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++){
        velocities[i] += dt * forces[i] / (densities[i] + 0.001f);
        positions[i] += dt * velocities[i];
        if (positions[i].y < 0.0f){
            positions[i].y = EPSILON;
            velocities[i].y *= -0.8f;
        }
        else if (positions[i].y > 10.0f){
            positions[i].y = 10.0f - EPSILON;
            velocities[i].y *= -0.8f;
        }
        if (positions[i].x < 0.0f){
            positions[i].x = EPSILON;
            velocities[i].x *= -0.8f;
        }
        else if (positions[i].x > 10.0f){
            positions[i].x = 10.0f - EPSILON;
            velocities[i].x *= -0.8f;
        }
        if (positions[i].z < 0.0f){
            positions[i].z = EPSILON;
            velocities[i].z *= -0.8f;
        }
        else if (positions[i].z > 10.0f){
            positions[i].z = 10.0f - EPSILON;
            velocities[i].z *= -0.8f;
        }

        float speed = glm::length(velocities[i]) / 10.0f;
        colors[i] = glm::vec3(speed, 0.0, 1.0 - speed);
    }
}

void Particles::setPositions(glm::vec3* positions){
    this->positions = positions;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, num_points * sizeof(glm::vec3), positions);
    glBindVertexArray(0);

    for (int i = 0; i < num_points; i++){
        velocities[i] = glm::vec3(0.0f);
        colors[i] = glm::vec3(1.0f);
    }
}