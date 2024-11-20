#include "world.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GLFW/glfw3.h>

void world_init(World* world, GLFWwindow* window) {
    world->window = window;
    
    // Get window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Initialize camera
    camera_init(&world->camera, width, height);
    
    // Initialize grid
    grid_init(&world->grid, 10.0f, 1.0f);
}

void world_render(World* world) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(&world->camera, view);
    camera_get_projection_matrix(&world->camera, projection);
    
    // Render grid
    grid_render(&world->grid, (float*)view, (float*)projection);

    // Calculate FPS and frame time
    static float fps = 0.0f;
    static float frameTime = 0.0f;
    static float fpsUpdateTimer = 0.0f;
    
    fpsUpdateTimer += deltaTime;
    if (fpsUpdateTimer >= 0.1f) {
        fps = 1.0f / deltaTime;
        frameTime = deltaTime * 1000.0f;
        fpsUpdateTimer = 0.0f;
    }
}

void world_cleanup(World* world) {
    if (!world) return;
    grid_cleanup(&world->grid);
}