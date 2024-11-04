#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include "grid.h"
#include "particle_system.h"
#include "camera.h"
#include "hud.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct World {
    GLFWwindow* window;
    Grid grid;
    ParticleSystem particles;
    Camera camera;
    HUD hud;
} World;

void world_init(World* world, GLFWwindow* window);
void world_render(World* world, Camera* camera);
void world_cleanup(World* world);
void screen_to_world_coords(double xpos, double ypos, Camera* camera, vec2 world_pos);

#ifdef __cplusplus
}
#endif

#endif // WORLD_H