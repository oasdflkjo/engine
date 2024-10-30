#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include "camera.h"

#define MAX_PARTICLES 10000000

typedef struct {
    // Grid stuff
    unsigned int gridVAO;
    unsigned int gridVBO;
    unsigned int gridShaderProgram;
    float gridSize;
    float gridSpacing;

    // Particle stuff
    unsigned int particleVAO;
    unsigned int positionBuffer;
    unsigned int velocityBuffer;
    unsigned int computeProgram;
    unsigned int renderProgram;
    int numParticles;
    vec2 mousePos;
    float deltaTime;
} World;

// Initialize the world (cube and shaders)
void world_init(World* world);

// Render the world (spinning cube)
void world_render(World* world, Camera* camera);

// Cleanup world resources
void world_cleanup(World* world);

// New functions for particle system
void world_update_particles(World* world);
void world_set_mouse_pos(World* world, float x, float y);
char* read_shader_file(const char* filename);

#endif 