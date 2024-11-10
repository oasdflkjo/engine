#include "world.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GLFW/glfw3.h>

void world_init(World* world, GLFWwindow* window, Simulation* simulation) {
    world->window = window;
    world->simulation = simulation;
    
    // Get window size
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    
    // Initialize camera
    camera_init(&world->camera, width, height);
    
    // Initialize grid
    grid_init(&world->grid, 10.0f, 1.0f);
    
    // Initialize simulation
    simulation_init(world->simulation);

    // Initialize HUD with window handle
    world->hud.window = window;
    hud_init(&world->hud, world->simulation);
}

void world_render(World* world, Camera* camera) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    
    world->simulation->deltaTime = deltaTime;

    // If you want to sync gravity point with camera target, do it here explicitly
    simulation_set_gravity_point(world->simulation, 
                               world->camera.target[0], 
                               world->camera.target[1]);

    // Update simulation
    simulation_update(world->simulation);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(&world->camera, view);
    camera_get_projection_matrix(&world->camera, projection);
    
    // Render grid
    grid_render(&world->grid, (float*)view, (float*)projection);

    // Render simulation
    simulation_render(world->simulation, view, projection);
    
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
    int particle_count = simulation_get_particle_count(world->simulation);
    hud_update_stats(&world->hud, fps, particle_count, frameTime, deltaTime);
    
    // Render HUD
    hud_render(&world->hud);
}

void world_cleanup(World* world) {
    grid_cleanup(&world->grid);
    simulation_cleanup(world->simulation);
    hud_cleanup(&world->hud);
}

void screen_to_world_coords(double xpos, double ypos, Camera* camera, vec2 world_pos) {
    // Convert screen coordinates to normalized device coordinates (NDC)
    float x = (2.0f * xpos) / camera->width - 1.0f;
    float y = 1.0f - (2.0f * ypos) / camera->height;
    
    // Create NDC point (z = -1 for near plane)
    vec4 ndc = {x, y, -1.0f, 1.0f};
    
    // Get inverse view-projection matrix
    mat4 view, projection, view_proj, inv_view_proj;
    camera_get_view_matrix(camera, view);
    camera_get_projection_matrix(camera, projection);
    glm_mat4_mul(projection, view, view_proj);
    glm_mat4_inv(view_proj, inv_view_proj);
    
    // Transform to world space
    vec4 world;
    glm_mat4_mulv(inv_view_proj, ndc, world);
    world[0] /= world[3];
    world[1] /= world[3];
    
    // Get ray direction from camera to clicked point
    vec3 ray_dir;
    ray_dir[0] = world[0] - camera->position[0];
    ray_dir[1] = world[1] - camera->position[1];
    ray_dir[2] = world[2] - camera->position[2];
    glm_vec3_normalize(ray_dir);
    
    // Find intersection with z=0 plane
    float t = -camera->position[2] / ray_dir[2];
    world_pos[0] = camera->position[0] + ray_dir[0] * t;
    world_pos[1] = camera->position[1] + ray_dir[1] * t;
}