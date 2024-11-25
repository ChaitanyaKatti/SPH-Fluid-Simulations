#include <cuda_runtime.h>
#include <thrust/device_vector.h>
#include <thrust/host_vector.h>
#include <curand_kernel.h>
#include <particlesGPU.cuh>

// Device helper functions
__device__ float d_poly6Kernel(float r2)
{
    const float h = KERNEL_H;
    const float h2 = h * h;
    if (r2 > h2)
        return 0.0f;
    float coefficient = 315.0f / (64.0f * M_PI * powf(h, 9));
    float term = h2 - r2;
    return coefficient * term * term * term;
}

__device__ float3 d_spikyGradient(float3 r, float r_len)
{
    const float h = KERNEL_H;
    if (r_len > h || r_len == 0.0f)
        return make_float3(0.0, 0.0f, 0.0f);
    float coefficient = -45.0f / (M_PI * powf(h, 6));
    float term = (h - r_len) * (h - r_len);
    return make_float3(
        coefficient * term * r.x / r_len,
        coefficient * term * r.y / r_len,
        coefficient * term * r.z / r_len);
}

__device__ float d_viscosityLaplacian(float r)
{
    const float h = KERNEL_H;
    if (r > h)
        return 0.0f;
    float coefficient = 45.0f / (M_PI * powf(h, 6));
    return coefficient * (h - r);
}

__device__ int d_hashCoords(int3 coords)
{
    const int p1 = 73856093;
    const int p2 = 19349663;
    const int p3 = 83492791;
    return ((coords.x * p1) ^ (coords.y * p2) ^ (coords.z * p3)) % HASH_SIZE;
}

__device__ int3 d_getCellId(float3 position, float cellSize)
{
    return make_int3(
        floor(position.x / cellSize),
        floor(position.y / cellSize),
        floor(position.z / cellSize));
}

__device__ void d_boundPosition(float3 &position, float3 &velocity, float3 bounds)
{
    // X boundaries
    if (position.x < 0.0f)
    {
        position.x = 0.0f;
        velocity.x = -COEFF_RESTITUTION * velocity.x;
    }
    else if (position.x > bounds.x)
    {
        position.x = bounds.x;
        velocity.x = -COEFF_RESTITUTION * velocity.x;
    }

    // Y boundaries
    if (position.y < 0.0f)
    {
        position.y = 0.0f;
        velocity.y = -COEFF_RESTITUTION * velocity.y;
    }
    else if (position.y > bounds.y)
    {
        position.y = bounds.y;
        velocity.y = -COEFF_RESTITUTION * velocity.y;
    }

    // Z boundaries
    if (position.z < 0.0f)
    {
        position.z = 0.0f;
        velocity.z = -COEFF_RESTITUTION * velocity.z;
    }
    else if (position.z > bounds.z)
    {
        position.z = bounds.z;
        velocity.z = -COEFF_RESTITUTION * velocity.z;
    }
}

// CUDA Kernels
__global__ void initParticlesKernel(
    float3 *positions,
    float3 *velocities,
    float3 *colors,
    float *densities,
    float *pressures,
    curandState *states,
    int num_points,
    float3 bounds)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_points)
        return;

    // Initialize random state
    curand_init(clock64(), idx, 0, &states[idx]);

    // Initialize position within bounds
    positions[idx].x = curand_uniform(&states[idx]) * bounds.x;
    positions[idx].y = curand_uniform(&states[idx]) * bounds.y;
    positions[idx].z = curand_uniform(&states[idx]) * bounds.z;

    // Initialize velocity (zero or small random values)
    velocities[idx] = make_float3(0.0f, 0.0f, 0.0f);

    // Initialize color (e.g., blue)
    colors[idx] = make_float3(0.0f, 0.0f, 1.0f);

    // Initialize density and pressure
    densities[idx] = 0.0f;
    pressures[idx] = 0.0f;
}

__global__ void updateGridKernel(
    const float3 *positions,
    int *particleMap,
    int *cellStart,
    int *cellEnd,
    int num_points,
    float cellSize)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_points)
        return;

    int3 cellId = d_getCellId(positions[idx], cellSize);
    int hash = d_hashCoords(cellId);

    // Update particle map
    particleMap[idx] = idx;

    // Update cell start and end indices atomically
    atomicMin(&cellStart[hash], idx);
    atomicMax(&cellEnd[hash], idx + 1);
}

__global__ void computeDensityPressureKernel(
    const float3 *positions,
    float *densities,
    float *pressures,
    float3 *colors,
    const int *particleMap,
    const int *cellStart,
    const int *cellEnd,
    int num_points,
    float mass,
    float restingDensity,
    float stiffness,
    float cellSize)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_points)
        return;

    float density = 0.0f;
    float3 pos_i = positions[idx];
    int3 cellId_i = d_getCellId(pos_i, cellSize);

    // Neighbor search in 27 neighboring cells
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int z = -1; z <= 1; z++)
            {
                int3 neighborCell = make_int3(
                    cellId_i.x + x,
                    cellId_i.y + y,
                    cellId_i.z + z);

                int neighborHash = d_hashCoords(neighborCell);

                // Iterate through particles in neighboring cell
                for (int j = cellStart[neighborHash]; j < cellEnd[neighborHash]; j++)
                {
                    int neighIdx = particleMap[j];
                    if (neighIdx == idx)
                        continue;

                    float3 r;
                    r.x = pos_i.x - positions[neighIdx].x;
                    r.y = pos_i.y - positions[neighIdx].y;
                    r.z = pos_i.z - positions[neighIdx].z;

                    float r2 = r.x * r.x + r.y * r.y + r.z * r.z;

                    if (r2 < KERNEL_H * KERNEL_H)
                    {
                        density += mass * d_poly6Kernel(r2);
                        colors[idx] = make_float3(1.0f, 1.0f, 1.0f);
                        colors[neighIdx] = make_float3(1.0f, 1.0f, 1.0f);
                    }
                }
            }
        }
    }

    densities[idx] = density;
    pressures[idx] = stiffness * (density - restingDensity);
}

__global__ void computeForcesKernel(
    const float3 *positions,
    const float3 *velocities,
    float3 *forces,
    const float *densities,
    const float *pressures,
    const int *particleMap,
    const int *cellStart,
    const int *cellEnd,
    int num_points,
    float mass,
    float viscosity,
    float cellSize)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_points)
        return;

    float3 force = make_float3(0.0f, -GRAVITY * mass, 0.0f); // Gravity
    float3 pos_i = positions[idx];
    float3 vel_i = velocities[idx];
    int3 cellId_i = d_getCellId(pos_i, cellSize);

    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            for (int z = -1; z <= 1; z++)
            {
                int3 neighborCell = make_int3(
                    cellId_i.x + x,
                    cellId_i.y + y,
                    cellId_i.z + z);

                int neighborHash = d_hashCoords(neighborCell);

                for (int j = cellStart[neighborHash]; j < cellEnd[neighborHash]; j++)
                {
                    int neighIdx = particleMap[j];
                    if (neighIdx == idx)
                        continue;

                    float3 r;
                    r.x = pos_i.x - positions[neighIdx].x;
                    r.y = pos_i.y - positions[neighIdx].y;
                    r.z = pos_i.z - positions[neighIdx].z;

                    float r_len = sqrtf(r.x * r.x + r.y * r.y + r.z * r.z);

                    if (r_len < KERNEL_H)
                    {
                        // Pressure force
                        float3 pressure_force = d_spikyGradient(r, r_len);
                        pressure_force.x *= -mass * (pressures[idx] + pressures[neighIdx]) / (2.0f * densities[neighIdx] + EPSILON);
                        pressure_force.y *= -mass * (pressures[idx] + pressures[neighIdx]) / (2.0f * densities[neighIdx] + EPSILON);
                        pressure_force.z *= -mass * (pressures[idx] + pressures[neighIdx]) / (2.0f * densities[neighIdx] + EPSILON);
                        force.x += pressure_force.x;
                        force.y += pressure_force.y;
                        force.z += pressure_force.z;

                        // Viscosity force
                        float3 vel_diff;
                        vel_diff.x = velocities[neighIdx].x - vel_i.x;
                        vel_diff.y = velocities[neighIdx].y - vel_i.y;
                        vel_diff.z = velocities[neighIdx].z - vel_i.z;

                        float visc_term = viscosity * mass * d_viscosityLaplacian(r_len) / (densities[neighIdx] + EPSILON);
                        force.x += visc_term * vel_diff.x;
                        force.y += visc_term * vel_diff.y;
                        force.z += visc_term * vel_diff.z;
                    }
                }
            }
        }
    }

    forces[idx] = force;
}

__global__ void integrateParticlesKernel(
    float3 *positions,
    float3 *velocities,
    const float3 *forces,
    const float *densities,
    float dt,
    float3 bounds)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_points)
        return;

    float3 velocity = velocities[idx];
    float3 position = positions[idx];
    float3 force = forces[idx];
    float density = densities[idx];

    // Update velocity
    velocity.x += dt * force.x / (density + EPSILON);
    velocity.y += dt * force.y / (density + EPSILON);
    velocity.z += dt * force.z / (density + EPSILON);

    // Limit velocity
    float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
    if (speed > MAX_VELOCITY)
    {
        float scale = MAX_VELOCITY / speed;
        velocity.x *= scale;
        velocity.y *= scale;
        velocity.z *= scale;
    }

    // Update position
    position.x += dt * velocity.x;
    position.y += dt * velocity.y;
    position.z += dt * velocity.z;

    // Handle boundaries
    d_boundPosition(position, velocity, bounds);

    // Write back results
    positions[idx] = position;
    velocities[idx] = velocity;
}

// Host wrapper functions
void ParticlesGPU::update(float dt)
{
    const int threadsPerBlock = 256;
    const int numBlocks = (num_points + threadsPerBlock - 1) / threadsPerBlock;

    // Reset grid
    cudaMemset(d_cellStart, INT_MAX, HASH_SIZE * sizeof(int));
    cudaMemset(d_cellEnd, 0, HASH_SIZE * sizeof(int));

    // Update spatial grid
    updateGridKernel<<<numBlocks, threadsPerBlock>>>(
        positions,
        particleMap,
        cellStart,
        cellEnd,
        num_points,
        cellSize);

    // Compute density and pressure
    computeDensityPressureKernel<<<numBlocks, threadsPerBlock>>>(
        positions,
        densities,
        pressures,
        colors,
        particleMap,
        cellStart,
        cellEnd,
        num_points,
        mass,
        restingDensity,
        stiffness,
        cellSize);

    // Compute forces
    computeForcesKernel<<<numBlocks, threadsPerBlock>>>(
        positions,
        velocities,
        forces,
        densities,
        pressures,
        particleMap,
        cellStart,
        cellEnd,
        num_points,
        mass,
        viscosity,
        cellSize);

    // Integrate particles
    integrateParticlesKernel<<<numBlocks, threadsPerBlock>>>(
        positions,
        velocities,
        forces,
        densities,
        dt,
        bounds);

    // Check for errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
    {
        std::cerr << "CUDA error: " << cudaGetErrorString(err) << std::endl;
    }

    // Synchronize device
    cudaDeviceSynchronize();
}