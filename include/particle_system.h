#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "cglm/cglm.h"

typedef struct {
    // Buffers
    unsigned int positionBuffer;
    unsigned int velocityBuffer;
    unsigned int velocityMagBuffer;
    unsigned int particleVAO;
    
    // Shaders
    unsigned int computeProgram;
    unsigned int renderProgram;
    
    // Particle data
    int numParticles;
    int count;
    float deltaTime;
    vec2 mousePos;
    
    // Legacy members (can be removed if not used)
    float* positions;
    float* velocities;
    float* colors;
    unsigned int VAO;
    unsigned int VBO;
    unsigned int shaderProgram;
    unsigned int computeShader;
    
    // Triple buffering buffers
    GLuint positionBuffers[3];    // Triple buffer for positions
    GLuint velocityBuffers[3];    // Triple buffer for velocities
    GLuint velocityMagBuffers[3]; // Triple buffer for velocity magnitudes
    
    // Buffer indices
    GLuint computeIndex;  // Buffer being written by compute shader
    GLuint renderIndex;   // Buffer being read by render
    GLuint nextIndex;     // Buffer ready for next frame
    
    // Sync objects
    GLsync fences[3];     // Sync fences for each buffer
} ParticleSystem;

void particle_system_init(ParticleSystem* ps);
void particle_system_update(ParticleSystem* ps);
void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection);
void particle_system_cleanup(ParticleSystem* ps);
void particle_system_set_mouse_pos(ParticleSystem* ps, float x, float y);

#endif // PARTICLE_SYSTEM_H 