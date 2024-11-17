#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <glad/glad.h>
#include <cglm/cglm.h>

typedef struct {
    GLuint positionBuffer;
    GLuint velocityBuffer;
    GLuint velocityMagBuffer;
    GLuint particleVAO;
} ParticleBuffers;

// Buffer initialization and management
ParticleBuffers create_particle_buffers(int numParticles, vec2* positions, vec2* velocities);
void destroy_particle_buffers(ParticleBuffers* buffers);

// SIMD-optimized particle data initialization
void init_particle_positions(vec2* positions, int numParticles);
void zero_particle_velocities(vec2* velocities, int numParticles);

#endif // BUFFER_MANAGER_H 