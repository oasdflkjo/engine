#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include "grid.h"
#include "particle_system.h"
#include "ui.h"
#include "hud.h"
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct World {
    GLFWwindow* window;
    Grid grid;
    ParticleSystem particles;
    UI ui;
    HUD hud;
} World;

void world_init(World* world, GLFWwindow* window);
void world_render(World* world, Camera* camera);
void world_cleanup(World* world);
void world_set_mouse_pos(World* world, float x, float y);

#ifdef __cplusplus
}
#endif

#endif // WORLD_H