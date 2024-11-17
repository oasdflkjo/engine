#include "world.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GLFW/glfw3.h>

void world_init(World* world, GLFWwindow* window, ParticleSystem* ps) {
    world->window = window;
    world->particle_system = ps;
    
    // Get window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Initialize camera
    camera_init(&world->camera, width, height);
    
    // Initialize grid
    grid_init(&world->grid, 10.0f, 1.0f);
    
    // Initialize particle system
    particle_system_init(world->particle_system);

    // Initialize HUD with window handle
    world->hud.window = window;
    hud_init(&world->hud, world->particle_system);
}

void world_render(World* world) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    world->particle_system->deltaTime = deltaTime;

    particle_system_set_gravity_point(world->particle_system, 
                                    world->camera.target[0], 
                                    world->camera.target[1]);

    // Update simulation
    particle_system_update(world->particle_system);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(&world->camera, view);
    camera_get_projection_matrix(&world->camera, projection);
    
    // Render grid
    grid_render(&world->grid, (float*)view, (float*)projection);

    // Render particles
    particle_system_render(world->particle_system, view, projection);
    
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
    
    // Update HUD stats
    hud_update_stats(&world->hud, fps, world->particle_system->count, frameTime, deltaTime);
    
    // Render HUD
    hud_render(&world->hud);
}

void world_cleanup(World* world) {
    grid_cleanup(&world->grid);
    particle_system_cleanup(world->particle_system);
    hud_cleanup(&world->hud);
}