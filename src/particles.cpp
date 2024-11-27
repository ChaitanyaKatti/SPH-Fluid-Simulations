#include <glad/glad.h>
#include <particles.hpp>
#include <config.hpp>
#include <bits/stdc++.h>

inline glm::vec3 getRandVec3()
{
    return glm::vec3((float)rand() / RAND_MAX, (float)rand() / RAND_MAX, (float)rand() / RAND_MAX);
}

void genUniformVec3Array(glm::vec3 *arr, int n, float scale = 1.0f)
{
    if (n <= 0)
    {
        return;
    }
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < n; j++)
        {
            for (int k = 0; k < n; k++)
            {
                arr[i * n * n + j * n + k] = glm::vec3(i, j, k) * (scale / (n - 1)) + 0.01f * getRandVec3();
            }
        }
    }
}

inline float poly6Kernel(float r2)
{
    if (r2 < h2)
    {
        return (315.0f / (64.0f * M_PI * h9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r)
{
    if (sqrt_r < h)
    {
        if (sqrt_r < 0.0001f)
        {
            return (float)(-45.0f / (M_PI * h6) * h2) * glm::normalize(getRandVec3());
        }
        return (float)(-45.0f / (M_PI * h6) * glm::pow(h - sqrt_r, 2)) * r / (sqrt_r + DIVISON_EPSILON);
    }
    return glm::vec3(0.0f);
}

inline float viscosityLaplacian(float sqrt_r)
{
    if (sqrt_r < h)
    {
        return 45.0f / (M_PI * h6) * (h - sqrt_r);
    }
    return 0.0f;
}

inline int hash(glm::vec3 p)
{
    int hash = (int(p.x / h) * 92837111) ^ (int(p.y / h) * 6892287499) ^ (int(p.z / h) * 283923481);
    hash = abs(hash) % HashTableSize;
    return hash;
}

Particles::Particles(Shader *const shader) : shader(shader)
{
    // Initialize arrays for SPH
    this->positions = new glm::vec3[NUM_INS];
    this->colors = new glm::vec3[NUM_INS];
    std::cout << "Volume: " << pow(MASS * NUM_INS / RESTING_DENSITY, 1.0 / 3.0) << std::endl;
    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);

    for (int i = 0; i < NUM_INS; i++)
    {
        colors[i] = glm::vec3(1.0f);
    }
    this->densities = new float[NUM_INS];      // Density
    this->pressures = new float[NUM_INS];      // Pressure
    this->forces = new glm::vec3[NUM_INS];     // Forces
    this->velocities = new glm::vec3[NUM_INS]; // Velocities
    // Arrays for hash table
    startIndex = new int[HashTableSize];
    stopIndex = new int[HashTableSize];
    indexArray = new int[NUM_INS];

    // Rendering
    setupVAO();
}

void Particles::setupVAO()
{
    // VAO : Vertex Array Object
    glGenVertexArrays(1, &VAO);
    // VBO : Vertex Buffer Object
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3) * 2, nullptr, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), colors);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void *)(NUM_INS * sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Particles::update()
{
    updateHash();
    calculateDensityAndPressure();
    applyForces();
    resolveCollisions();

    // Update VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Particles::updateHash()
{
    // Initialize all arrays to zero
    std::fill(startIndex, startIndex + HashTableSize, 0);
    std::fill(stopIndex, stopIndex + HashTableSize, 0);
    std::fill(indexArray, indexArray + NUM_INS, 0);

    // For each particle, find its cell Id and add to count array
    // #pragma omp parallel for shared(stopIndex)
    for (int i = 0; i < NUM_INS; i++)
    {
        int cellId = hash(positions[i]);
        stopIndex[cellId]++;
    }

    // Do partial sum and store in startIndex and stopIndex
    for (int i = 1; i < HashTableSize; i++)
    {
        stopIndex[i] += stopIndex[i - 1];
    }
    // copy to start index
    std::copy_n(stopIndex, HashTableSize, startIndex);

    // For each particle, find its cell Id and add
    // #pragma omp parallel for shared(startIndex)
    for (int i = 0; i < NUM_INS; i++)
    {
        int cellId = hash(positions[i]);
        startIndex[cellId]--;
        indexArray[startIndex[cellId]] = i;
    }
}

void Particles::calculateDensityAndPressure()
{
#pragma omp parallel for shared(densities, pressures)
    /// N^2 algorithm
    for (int i = 0; i < NUM_INS; i++)
    {
        densities[i] = 0.0f;
        for (int j = 0; j < NUM_INS; j++)
        {
            if (i == j)
                continue;
            glm::vec3 r = positions[j] - positions[i];
            float r2 = glm::dot(r, r);
            densities[i] += MASS * poly6Kernel(r2);
        }
        pressures[i] = k * std::max((densities[i] - RESTING_DENSITY), 0.0f);
    }
}

void Particles::applyForces()
{
    // N^2 algorithm
#pragma omp parallel for
    for (int i = 0; i < NUM_INS; i++)
    {
        forces[i] = glm::vec3(0.0f, -MASS * g, 0.0f); // Gravity

        for (int j = 0; j < NUM_INS; j++)
        {
            if (i == j)
                continue;
            glm::vec3 r = positions[i] - positions[j];
            float sqrt_r = glm::length(r);
            forces[i] += -MASS * (pressures[i] + pressures[j]) / (2.0f * densities[j] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
            forces[i] += mu * MASS * (velocities[j] - velocities[i]) / (densities[j] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
        }

        // Update velocities and positions
        velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
        if (glm::length(velocities[i]) > MAX_VELOCITY)
        {
            velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
        }
        positions[i] += dt * velocities[i] + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

        // Apply boundary conditions
        if (positions[i].x < 0.0f)
        {
            positions[i].x = EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        else if (positions[i].x > 10.0f)
        {
            positions[i].x = 10.0f - EPSILON;
            velocities[i].x *= -COEFF_RESTITUTION;
        }
        if (positions[i].y < 0.0f)
        {
            positions[i].y = EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        else if (positions[i].y > 10.0f)
        {
            positions[i].y = 10.0f - EPSILON;
            velocities[i].y *= -COEFF_RESTITUTION;
        }
        if (positions[i].z < 0.0f)
        {
            positions[i].z = EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
        }
        else if (positions[i].z > 5.0f)
        {
            positions[i].z = 5.0f - EPSILON;
            velocities[i].z *= -COEFF_RESTITUTION;
        }

        // Update colors
        float speed = pow(glm::length(velocities[i])/MAX_VELOCITY, 0.4f);
        colors[i].x = speed;
        colors[i].y = (1.0f - speed);
        colors[i].z = 4*colors[i].x * colors[i].y;
    }
}

void Particles::resolveCollisions()
{
    // Particle-Particle collisions
    for (int i = 0; i < NUM_INS; i++)
    {
#pragma omp parallel for
        for (int j = i + 1; j < NUM_INS; j++)
        {
            float dist = glm::length(positions[i] - positions[j]);
            if (dist < 2.0f * Radius)
            {
                glm::vec3 normal = glm::normalize(positions[i] - positions[j]);
                positions[i] += normal * (Radius - 0.5f * dist);
                positions[j] -= normal * (Radius - 0.5f * dist);
                velocities[i] -= glm::dot(velocities[i], normal) * normal * (1 + COEFF_RESTITUTION);
                velocities[j] -= glm::dot(velocities[j], normal) * normal * (1 + COEFF_RESTITUTION);
            }
        }
    }
}

void Particles::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", Radius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

void Particles::setPositions(glm::vec3 *positions)
{
    this->positions = positions;
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBindVertexArray(0);

    for (int i = 0; i < NUM_INS; i++)
    {
        velocities[i] = glm::vec3(0.0f);
        colors[i] = glm::vec3(1.0f);
    }
}

void Particles::reset()
{
    genUniformVec3Array(positions, NUM_INS_DIM, 5.0f);
    for (int i = 0; i < NUM_INS; i++)
    {
        colors[i] = glm::vec3(1.0f);
        velocities[i] = glm::vec3(0.0f);
        forces[i] = glm::vec3(0.0f);
    }
}

Particles::~Particles()
{
    delete[] positions;
    delete[] colors;
    delete[] densities;
    delete[] pressures;
    delete[] forces;
    delete[] velocities;
    delete[] startIndex;
    delete[] stopIndex;
    delete[] indexArray;
}