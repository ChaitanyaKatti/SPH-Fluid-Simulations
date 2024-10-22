#pragma once

// Particle parameters
inline const int NUM_INS_DIM = 10;
inline const int NUM_INS = NUM_INS_DIM * NUM_INS_DIM * NUM_INS_DIM;

// SPH parameters
inline float MASS = 1.0f;              // Particle mass
inline float dt = 0.005f;              // Time step
inline float h = 1.0f;                 // Smoothing length
inline float h2 = h * h;               // Smoothing length squared
inline float h6 = h2 * h2 * h2;        // Smoothing length to the power of 6
inline float k = 20.0f;                // Stiffness
inline float mu = 1.0f;                // Viscosity
inline float g = 9.81f;                // Gravity
inline float EPSILON = 0.001f;         // Boundary epsilon
inline float DIVISON_EPSILON = 0.01f;  // Division epsilon
inline float COEFF_RESTITUTION = 0.8f; // Restitution coefficient
inline float MAX_VELOCITY = 10.0f;     // Maximum velocity of particles
inline float DENSITY = 1.0f;          // Resting density
// Window size
inline unsigned int SCR_WIDTH = 1200;
inline unsigned int SCR_HEIGHT = 800;