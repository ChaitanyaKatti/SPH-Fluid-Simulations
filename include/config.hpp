#pragma once

// Particle parameters
#define NUM_INS_DIM 15
#define NUM_INS (NUM_INS_DIM * NUM_INS_DIM * NUM_INS_DIM)

// SPH parameters
#define MASS 1.0f              // Particle mass
#define RESTING_DENSITY 9.0f   // Resting density
#define dt 0.01f             // Time step
#define h1 1.0f               // Smoothing length
#define h2 h1 *h1              // Smoothing length squared
#define h6 h2 *h2 *h2          // Smoothing length to the power of 6
#define h9 h6 *h2 *h1          // Smoothing length to the power of 9
#define BULK_MODULUS 1000.0f   // Stiffness
#define mu 0.5f                // Viscosity
#define gravity 9.81f          // Gravity
#define EPSILON 0.0001f        // Boundary epsilon
#define DIVISON_EPSILON 0.01f  // Division epsilon
#define COEFF_RESTITUTION 0.8f // Restitution coefficient
#define MAX_VELOCITY 100.0f    // Maximum velocity of particles
#define Radius 0.05f           // Particle radius for collision detection

// Window size
inline unsigned int SCR_WIDTH = 1200;
inline unsigned int SCR_HEIGHT = 800;