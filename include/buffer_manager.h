#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <glad/glad.h>
#include <cglm/cglm.h>

typedef struct {
    GLuint particleBuffer;
    GLuint visibleParticleBuffer;
    GLuint drawCommandBuffer;
    GLuint particleVAO;
} ParticleBuffers;

// Buffer initialization and management
ParticleBuffers create_particle_buffers(int numParticles, vec4* particleData);
void destroy_particle_buffers(ParticleBuffers* buffers);

// Initialize packed particle data: xy = position, zw = velocity
void init_particle_data(vec4* particleData, int numParticles);

#endif // BUFFER_MANAGER_H 
