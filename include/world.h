#ifndef WORLD_H
#define WORLD_H

#include "camera.h"
#include "grid.h"
#include "hud.h"
#include "simulation.h"

typedef struct World {
    GLFWwindow* window;
    Camera camera;
    Grid grid;
    HUD hud;
    Simulation* simulation;  // Now uses the abstract interface
} World;

void world_init(World* world, GLFWwindow* window, Simulation* simulation);
void world_render(World* world, Camera* camera);
void world_cleanup(World* world);

#endif // WORLD_H