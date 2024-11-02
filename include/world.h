#ifndef WORLD_H
#define WORLD_H

#include "grid.h"
#include "camera.h"
#include "quadtree.h"

#define MAX_PARTICLES 100000

typedef struct {
    Grid grid;
    unsigned int computeProgram;
    unsigned int renderProgram;
    unsigned int positionBuffer;
    unsigned int velocityBuffer;
    unsigned int velocityMagBuffer;
    unsigned int particleVAO;
    int numParticles;
    float deltaTime;
    QuadTree quadtree;
} World;

// Function declarations
void world_init(World* world);
void world_cleanup(World* world);
void world_render(World* world, Camera* camera);
void world_update_particles(World* world);

#endif