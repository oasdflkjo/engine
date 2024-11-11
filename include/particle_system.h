#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "simulation.h"
#include <cglm/cglm.h>

// Forward declare OpenGL types
typedef unsigned int GLuint;
typedef int GLint;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // Basic particle system properties
    int numParticles;
    int count;
    float deltaTime;
    vec2 gravityPoint;

    // OpenGL objects
    GLuint positionBuffer;
    GLuint velocityBuffer;
    GLuint velocityMagBuffer;
    GLuint particleVAO;
    GLuint computeProgram;
    GLuint renderProgram;

    // Physics parameters
    float minDistance;
    float forceScale;
    float maxForce;
    float terminalVelocity;
    float damping;
    float mouseForceRadius;
    float mouseForceStrength;

    // Uniform locations
    GLint deltaTimeLocation;
    GLint gravityPointLocation;
    GLint numParticlesLocation;
    GLint particleOffsetLocation;
    GLint batchSizeLocation;
    GLint minDistanceLocation;
    GLint forceScaleLocation;
    GLint maxForceLocation;
    GLint terminalVelocityLocation;
    GLint dampingLocation;
    GLint mouseForceRadiusLocation;
    GLint mouseForceStrengthLocation;

    // Additional parameters
    float attractionStrength;
    float timeScale;

    // Add uniform locations
    GLint attractionStrengthLocation;
    GLint timeScaleLocation;

    // Frustum culling
    vec4 frustumPlanes[6];  // For frustum culling
} ParticleSystem;

void particle_system_init(ParticleSystem* ps);
void particle_system_update(ParticleSystem* ps);
void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection);
void particle_system_cleanup(ParticleSystem* ps);
void particle_system_set_gravity_point(ParticleSystem* ps, float x, float y);

// Parameter setter functions
void particle_system_set_force_scale(ParticleSystem* ps, float scale);
void particle_system_set_damping(ParticleSystem* ps, float damping);
void particle_system_set_terminal_velocity(ParticleSystem* ps, float velocity);
void particle_system_set_attraction_strength(ParticleSystem* ps, float strength);
void particle_system_set_time_scale(ParticleSystem* ps, float scale);

// Add these new function declarations
Simulation* create_particle_simulation(void);
void destroy_particle_simulation(Simulation* sim);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_SYSTEM_H 