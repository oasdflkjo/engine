#include "world.h"
#include "shader.h"
#include "ui.h"
#include "hud.h"
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

    // Initialize HUD
    world->hud.window = window;
    hud_init(&world->hud, &world->particles);

    // Initialize UI
    ui_init(&world->ui, window);
}

void world_render(World* world, Camera* camera) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    world->particles.deltaTime = deltaTime;

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
    
    // Calculate FPS and frame time
    static float fps = 0.0f;
    static float frameTime = 0.0f;
    static float fpsUpdateTimer = 0.0f;
    
    fpsUpdateTimer += deltaTime;
    if (fpsUpdateTimer >= 0.1f) { // Update every 0.1 seconds
        fps = 1.0f / deltaTime;
        frameTime = deltaTime * 1000.0f;
        fpsUpdateTimer = 0.0f;
    }
    
    // Update HUD stats
    hud_update_stats(&world->hud, fps, world->particles.count, frameTime, deltaTime);
    
    // Start ImGui frame and render UI components
    ui_render(&world->ui, world);  // Always render base UI
    
    // Only render HUD if UI is visible
    if (ui_is_visible(&world->ui)) {
        hud_render(&world->hud);
    }
    
    ui_end_frame();
}

void world_cleanup(World* world) {
    grid_cleanup(&world->grid);
    particle_system_cleanup(&world->particles);
    ui_cleanup(&world->ui);
    hud_cleanup(&world->hud);
}

void world_set_mouse_pos(World* world, float x, float y) {
    particle_system_set_mouse_pos(&world->particles, x, y);
}