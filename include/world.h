#ifndef WORLD_H
#define WORLD_H

#include "camera.h"
#include "grid.h"
#include "hud.h"
#include "particle_system.h"

typedef struct World {
    GLFWwindow* window;
    Camera camera;
    Grid grid;
    ParticleSystem* particle_system;
    HUD hud;
} World;

void world_init(World* world, GLFWwindow* window, ParticleSystem* ps);
/**
 * Renders the world and all its components
 * @param world Pointer to the world instance
 */
void world_render(World* world);
void world_cleanup(World* world);

#endif // WORLD_H