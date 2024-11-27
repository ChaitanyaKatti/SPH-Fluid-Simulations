#include <glad/glad.h>
#include <particles.hpp>
#include <config.hpp>

extern unsigned int SCR_WIDTH;
extern unsigned int SCR_HEIGHT;

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
                arr[i * n * n + j * n + k] = glm::vec3(i, j, k) * (scale / (n - 1));
            }
        }
    }
}

inline float poly6Kernel(float r2)
{
    if (r2 < h2)
    {
        return 315.0f / (64.0f * M_PI * glm::pow(h, 9)) * glm::pow(h2 - r2, 3);
    }
    return 0.0f;
}

inline glm::vec3 spikyGradient(glm::vec3 r, float sqrt_r)
{
    if (sqrt_r < h)
    {
        // if (sqrt_r < 0.0001f){
        // return (float)(-45.0f / (M_PI * h6) * h2 ) * glm::normalize(getRandVec3());
        // }
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

Particles::Particles(Shader *const shader) : shader(shader)
{
    // Initialize arrays for SPH
    this->positions = new glm::vec3[NUM_INS];
    this->colors = new glm::vec3[NUM_INS];
    genUniformVec3Array(positions, NUM_INS_DIM, pow(MASS*NUM_INS/RESTING_DENSITY, 1.0/3.0));
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

    // Update VBO
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, NUM_INS * sizeof(glm::vec3), positions);
    glBufferSubData(GL_ARRAY_BUFFER, NUM_INS * sizeof(glm::vec3), NUM_INS * sizeof(glm::vec3), colors);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

inline int Particles::hash(glm::vec3 p)
{
    int hash = (int(p.x / h) * 92837111) ^ (int(p.y / h) * 6892287499) ^ (int(p.z / h) * 283923481);
    hash = abs(hash) % HashTableSize;
    return hash;
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

void Particles::Draw()
{
    shader->use();
    shader->setMat4("modelMatrix", glm::mat4(1.0f));
    shader->setFloat("pointSize", RenderRadius);
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, NUM_INS);
    glBindVertexArray(0);
}

void Particles::calculateDensityAndPressure()
{
#pragma omp parallel for
    for (int i = 0; i < NUM_INS; i++)
    { // For each point
        densities[i] = 0.0f;
        // Search for neighboring 27 cells for particles
        for (int x = -1; x <= 1; x++)
        {
            if ((positions[i].x < 1 && x == -1) || (positions[i].x > 9 && x == 1))
                continue;
            for (int y = -1; y <= 1; y++)
            {
                if ((positions[i].y < 1 && y == -1) || (positions[i].y > 9 && y == 1))
                    continue;
                for (int z = -1; z <= 1; z++)
                {
                    if ((positions[i].z < 1 && z == -1) || (positions[i].z > 9 && z == 1))
                        continue;

                    int cellId = hash(positions[i] + glm::vec3(x, y, z) * h);
                    for (int j = startIndex[cellId]; j < stopIndex[cellId]; j++)
                    {
                        int neighborIndex = indexArray[j];
                        if (i != neighborIndex)
                        {
                            glm::vec3 r = positions[i] - positions[neighborIndex];
                            float r2 = glm::dot(r, r);
                            densities[i] += MASS * poly6Kernel(r2);
                        }
                    }
                }
            }
        }

        pressures[i] = k * max((densities[i] - RESTING_DENSITY), 0);
    }
}

void Particles::applyForces()
{
#pragma omp parallel for
    for (int i = 0; i < NUM_INS; i++)
    {
        forces[i] = glm::vec3(0.0f, -MASS * g, 0.0f); // Gravity

        for (int x = -1; x <= 1; x++)
        {
            if ((positions[i].x < 1 && x == -1) || (positions[i].x > 9 && x == 1))
                continue;
            for (int y = -1; y <= 1; y++)
            {
                if ((positions[i].y < 1 && y == -1) || (positions[i].y > 9 && y == 1))
                    continue;
                for (int z = -1; z <= 1; z++)
                {
                    if ((positions[i].z < 1 && z == -1) || (positions[i].z > 9 && z == 1))
                        continue;

                    int cellId = hash(positions[i] + glm::vec3(x, y, z) * h);
                    for (int j = startIndex[cellId]; j < stopIndex[cellId]; j++)
                    {
                        int neighborIndex = indexArray[j];
                        if (i != neighborIndex)
                        {
                            glm::vec3 r = positions[i] - positions[neighborIndex];
                            float sqrt_r = glm::length(r);
                            forces[i] += -MASS * (pressures[i] + pressures[neighborIndex]) / (2.0f * densities[neighborIndex] + DIVISON_EPSILON) * spikyGradient(r, sqrt_r);  // Pressure term
                            forces[i] += mu * MASS * (velocities[neighborIndex] - velocities[i]) / (densities[neighborIndex] + DIVISON_EPSILON) * viscosityLaplacian(sqrt_r); // Viscosity term
                        }
                    }
                }
            }
        }

        // Update velocities and positions
        velocities[i] += dt * forces[i] / (densities[i] + DIVISON_EPSILON);
        if (glm::length(velocities[i]) > MAX_VELOCITY)
        {
            velocities[i] = glm::normalize(velocities[i]) * MAX_VELOCITY;
        }
        positions[i] += dt * velocities[i]; // + 0.5f * dt * dt * forces[i] / (densities[i] + DIVISON_EPSILON);

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
        float speed = glm::length(velocities[i]) / 5.0f;
        colors[i].x = glm::clamp(1.0f - speed, 0.0f, 1.0f);
        colors[i].y = glm::clamp(1.0f - speed, 0.0f, 1.0f);
        colors[i].z = 1.0f;
    }
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
    setPositions(positions);
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