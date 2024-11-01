#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include "grid.h"
#include "camera.h"

#define MAX_PARTICLES 65000000  // 40 million particles

typedef float vec2[2];

typedef struct {
    // Grid component
    Grid grid;
    
    // Particle system components
    unsigned int numParticles;
    unsigned int positionBuffer;
    unsigned int velocityBuffer;
    unsigned int particleVAO;
    unsigned int computeProgram;
    unsigned int renderProgram;
    
    // Mouse and timing
    float mousePos[2];
    float deltaTime;
    
    // Add to the World struct:
    GLuint velocityMagBuffer;
} World;

void world_init(World* world);
void world_render(World* world, Camera* camera);
void world_cleanup(World* world);
void world_update_particles(World* world);
void world_set_mouse_pos(World* world, float x, float y);

#endif // WORLD_H