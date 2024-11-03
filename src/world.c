#include "world.h"
#include "shader.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <math.h>
#include <GLFW/glfw3.h>
#include "particle_system.h"

void world_init(World* world, GLFWwindow* window) {
    // Store window
    world->window = window;
    
    // Initialize grid
    grid_init(&world->grid, 10.0f, 1.0f);
    
    // Initialize particle system
    particle_system_init(&world->particles);
    
    // Initialize UI (now window is available)
    ui_init(&world->ui, world->window);
}

void world_render(World* world, Camera* camera) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    world->particles.deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Update particles
    particle_system_update(&world->particles);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(camera, view);
    camera_get_projection_matrix(camera, projection);
    
    // Render grid
    grid_render(&world->grid, (float*)view, (float*)projection);

    // Render particles
    particle_system_render(&world->particles, view, projection);
    
    // Render UI
    ui_render(&world->ui, world);
}

void world_cleanup(World* world) {
    grid_cleanup(&world->grid);
    particle_system_cleanup(&world->particles);
    ui_cleanup(&world->ui);
}

void world_set_mouse_pos(World* world, float x, float y) {
    particle_system_set_mouse_pos(&world->particles, x, y);
}