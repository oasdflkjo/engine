#ifndef WORLD_H
#define WORLD_H

#include "grid.h"
#include "particle_system.h"
#include "camera.h"
#include "ui.h"
#include "hud.h"

struct World {
    Grid grid;
    ParticleSystem particles;
    UI ui;
    HUD hud;
    GLFWwindow* window;
};

typedef struct World World;

void world_init(World* world, GLFWwindow* window);
void world_render(World* world, Camera* camera);
void world_cleanup(World* world);
void world_set_mouse_pos(World* world, float x, float y);

#endif // WORLD_H