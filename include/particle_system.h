#ifndef PARTICLE_SYSTEM_H
#define PARTICLE_SYSTEM_H

#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "cglm/cglm.h"

typedef struct {
    unsigned int positionBuffer;
    unsigned int velocityBuffer;
    unsigned int velocityMagBuffer;
    unsigned int particleVAO;
    unsigned int computeProgram;
    unsigned int renderProgram;
    int numParticles;
    vec2 mousePos;
    float deltaTime;
} ParticleSystem;

void particle_system_init(ParticleSystem* ps);
void particle_system_update(ParticleSystem* ps);
void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection);
void particle_system_cleanup(ParticleSystem* ps);
void particle_system_set_mouse_pos(ParticleSystem* ps, float x, float y);

#endif // PARTICLE_SYSTEM_H 