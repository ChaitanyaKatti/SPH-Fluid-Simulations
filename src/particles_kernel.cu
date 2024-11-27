#include <cuda_runtime.h>

__constant__ float d_h = 1.0f;
__constant__ float d_mass = 1.0f;
__constant__ float d_k = 1000.0f;
__constant__ float d_rho0 = 1000.0f;
__constant__ float d_mu = 0.1f;
__constant__ float d_g = 9.81f;
__constant__ float d_dt = 0.016f;

__device__ float length(float3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

__device__ float3 normalize(float3 v) {
    float len = length(v);
    return len > 0.0f ? make_float3(v.x / len, v.y / len, v.z / len) : make_float3(0.0f, 0.0f, 0.0f);
}

__device__ float3 operator-(const float3& a, const float3& b) {
    return make_float3(a.x - b.x, a.y - b.y, a.z - b.z);
}

__device__ float3 operator+(const float3& a, const float3& b) {
    return make_float3(a.x + b.x, a.y + b.y, a.z + b.z);
}

__device__ float3 operator*(const float3& a, float b) {
    return make_float3(a.x * b, a.y * b, a.z * b);
}

__device__ float3 operator/(const float3& a, float b) {
    return make_float3(a.x / b, a.y / b, a.z / b);
}

__device__ float3 operator*(float b, const float3& a) {
    return make_float3(a.x * b, a.y * b, a.z * b);
}

__device__ void operator+=(float3& a, const float3& b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

__device__ void operator*=(float3& a, float b) {
    a.x *= b;
    a.y *= b;
    a.z *= b;
}

__device__ float3 computePressureForce(float3 pos_i, float3 pos_j, float rho_i, float rho_j, float p_i, float p_j) {
    float3 r = pos_i - pos_j;
    float len = length(r);
    if (len < d_h && len > 0.0f) {
        return -d_mass * (p_i + p_j) / (2.0f * rho_j) * (d_h - len) * (d_h - len) * normalize(r) / len;
    }
    return make_float3(0.0f, 0.0f, 0.0f);
}

__device__ float3 computeViscosityForce(float3 pos_i, float3 pos_j, float3 vel_i, float3 vel_j, float rho_j) {
    float3 r = pos_i - pos_j;
    float len = length(r);
    if (len < d_h && len > 0.0f) {
        return d_mu * d_mass * (vel_j - vel_i) / rho_j * (d_h - len);
    }
    return make_float3(0.0f, 0.0f, 0.0f);
}

__global__ void computeForcesKernel(float3* pos, float3* vel, float3* acc, float* rho, float* p, int numParticles) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) return;

    float3 force = make_float3(0.0f, -d_g, 0.0f);
    float3 pos_i = pos[idx];
    float3 vel_i = vel[idx];
    float rho_i = rho[idx];
    float p_i = p[idx];

    for (int j = 0; j < numParticles; j++) {
        if (j == idx) continue;

        force += computePressureForce(pos_i, pos[j], rho_i, rho[j], p_i, p[j]);
        force += computeViscosityForce(pos_i, pos[j], vel_i, vel[j], rho[j]);
    }

    acc[idx] = force;
}

__global__ void integrationKernel(float3* pos, float3* vel, float3* acc, int numParticles) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numParticles) return;

    vel[idx] += acc[idx] * d_dt;
    pos[idx] += vel[idx] * d_dt;

    // Boundary conditions
    if (pos[idx].x < 0.0f) { pos[idx].x = 0.0f; vel[idx].x *= -0.5f; }
    if (pos[idx].x > 10.0f) { pos[idx].x = 10.0f; vel[idx].x *= -0.5f; }
    if (pos[idx].y < 0.0f) { pos[idx].y = 0.0f; vel[idx].y *= -0.5f; }
    if (pos[idx].y > 10.0f) { pos[idx].y = 10.0f; vel[idx].y *= -0.5f; }
    if (pos[idx].z < 0.0f) { pos[idx].z = 0.0f; vel[idx].z *= -0.5f; }
    if (pos[idx].z > 5.0f) { pos[idx].z = 5.0f; vel[idx].z *= -0.5f; }
}

extern "C" void launchUpdateParticles(float* d_pos, float* d_vel, float* d_acc, int numParticles, float deltaTime) {
    dim3 block(256);
    dim3 grid((numParticles + block.x - 1) / block.x);

    float3* pos = reinterpret_cast<float3*>(d_pos);
    float3* vel = reinterpret_cast<float3*>(d_vel);
    float3* acc = reinterpret_cast<float3*>(d_acc);

    // Allocate temporary arrays for density and pressure
    float *d_rho, *d_p;
    cudaMalloc(&d_rho, numParticles * sizeof(float));
    cudaMalloc(&d_p, numParticles * sizeof(float));

    computeForcesKernel<<<grid, block>>>(pos, vel, acc, d_rho, d_p, numParticles);
    integrationKernel<<<grid, block>>>(pos, vel, acc, numParticles);

    cudaFree(d_rho);
    cudaFree(d_p);
}
