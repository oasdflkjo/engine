#ifndef WORLD_H
#define WORLD_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "camera.h"
#include "grid.h"

typedef struct World {
    GLFWwindow* window;
    Camera camera;
    Grid grid;
} World;

void world_init(World* world, GLFWwindow* window);
/**
 * Renders the world and all its components
 * @param world Pointer to the world instance
 */
void world_render(World* world);
void world_cleanup(World* world);

#endif // WORLD_H
