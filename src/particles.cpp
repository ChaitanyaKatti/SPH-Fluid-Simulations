#include <glad/glad.h>
#include <iostream>
#include <particles.hpp>
#include <utils.hpp>

#define h 1.0f
#define h2 h * h
#define h6 h2 * h2 * h2
#define k 20.0f
#define mu 0.5f
#define g 9.81f
#define dt 0.01f
#define EPSILON 0.001f
#define DIVISON_EPSILON 0.01f
#define COEFF_RESTITUTION 0.8f
#define MAX_VELOCITY 10.0f

inline float poly6Kernel(float r2){
    if (r2 < h2){
        return 315.0f / (64.0f * M_PI * glm::pow(h, 9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r){
    if (sqrt_r < h){
        // if (sqrt_r < 0.0001f){
            // return (float)(-45.0f / (M_PI * h6) * h2 ) * glm::normalize(getRandVec3());
        // }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

inline float viscosityLaplacian(float sqrt_r){
    if (sqrt_r < h){
        return 45.0f / (M_PI * h6) * (h - sqrt_r);
    }
    return 0.0f;
}

Particles::Particles(const float mass, const float resting_density, const float radius, glm::vec3* positions, glm::vec3* colors, int num_points, Shader* const shader) : mass(mass), resting_density(resting_density), radius(radius), positions(positions), colors(colors), num_points(num_points), shader(shader) {
    setupParticles();
}

void Particles::setupParticles(){
    this->velocities = new glm::vec3[num_points];
    for (int i = 0; i < num_points; i++){
        velocities[i] = glm::vec3(0.0f);
    }
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
    calculateDensityAndPressure();
    applyForces();
    // std::cout << "positions " << positions[0].x << " " << positions[0].y << " " << positions[0].z << std::endl;
    // std::cout << "forces    "  << forces[0].x << " " << forces[0].y << " " << forces[0].z << std::endl;
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

void Particles::calculateDensityAndPressure(){
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
    
        pressures[i] = k * (densities[i] - resting_density);
    }
}

void Particles::applyForces(){
// #pragma omp parallel for
    for (int i = 0; i < num_points; i++){
        // glm::vec3 f_pressure = glm::vec3(0.0f);
        // glm::vec3 f_viscosity = glm::vec3(0.0f);
        // glm::vec3 f_gravity = glm::vec3(0.0f, -mass * g, 0.0f);
        forces[i] = glm::vec3(0.0f, -mass * g, 0.0f);
        
        for (int j = 0; j < num_points; j++){
            if (i != j){
                glm::vec3 r = positions[i] - positions[j];
                float sqrt_r = glm::length(r);
                // f_pressure += -mass * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);
                // f_viscosity += mu * mass * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r);
                forces[i] += -mass * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);
                forces[i] += mu * mass * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r);
            }
        }
        
        // forces[i] = f_pressure + f_viscosity + f_gravity;
    
        // Apply external forces
        velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
        if (glm::length(velocities[i]) > MAX_VELOCITY){
            velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
        }
        positions[i] += dt * velocities[i];
        if (positions[i].y < 0.0f){
            positions[i].y = EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        else if (positions[i].y > 10.0f){
            positions[i].y = 10.0f - EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        if (positions[i].x < 0.0f){
            positions[i].x = EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        else if (positions[i].x > 10.0f){
            positions[i].x = 10.0f - EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        if (positions[i].z < 0.0f){
            positions[i].z = EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
        }
        else if (positions[i].z > 10.0f){
            positions[i].z = 10.0f - EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
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