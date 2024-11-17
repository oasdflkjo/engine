#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "shader_program.h"
#include <cglm/cglm.h>

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
    
    // Shader programs
    ShaderProgram* computeProgram;
    ShaderProgram* renderProgram;

    // Physics parameters
    float minDistance;
    float forceScale;
    float maxForce;
    float terminalVelocity;
    float damping;
    float mouseForceRadius;
    float mouseForceStrength;
    float attractionStrength;
    float timeScale;

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
    GLint attractionStrengthLocation;
    GLint timeScaleLocation;
} ParticleSystem;

// Core functions
ParticleSystem* particle_system_create(void);
void particle_system_init(ParticleSystem* ps);
void particle_system_update(ParticleSystem* ps);
void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection);
void particle_system_cleanup(ParticleSystem* ps);
void particle_system_destroy(ParticleSystem* ps);

// Parameter getters/setters
void particle_system_set_gravity_point(ParticleSystem* ps, float x, float y);
void particle_system_set_force_scale(ParticleSystem* ps, float scale);
void particle_system_set_damping(ParticleSystem* ps, float damping);
void particle_system_set_terminal_velocity(ParticleSystem* ps, float velocity);
void particle_system_set_attraction_strength(ParticleSystem* ps, float strength);
void particle_system_set_time_scale(ParticleSystem* ps, float scale);
float particle_system_get_time_scale(ParticleSystem* ps);
float particle_system_get_attraction_strength(ParticleSystem* ps);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_SYSTEM_H
