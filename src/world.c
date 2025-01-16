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

    // Get mouse position and convert it to world space
    double mouseX, mouseY;
    glfwGetCursorPos(world->window, &mouseX, &mouseY);
    
    // Convert screen coordinates to world coordinates
    int windowWidth, windowHeight;
    glfwGetWindowSize(world->window, &windowWidth, &windowHeight);
    
    // Convert to normalized device coordinates (-1 to 1)
    float ndcX = (2.0f * mouseX) / windowWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / windowHeight; // Flip Y coordinate
    
    // Get matrices from camera
    mat4 view, projection;
    camera_get_view_matrix(&world->camera, view);
    camera_get_projection_matrix(&world->camera, projection);
    
    // Combined inverse matrix
    mat4 invMatrix;
    mat4 viewProj;
    glm_mat4_mul(projection, view, viewProj);
    glm_mat4_inv(viewProj, invMatrix);
    
    // Transform point
    vec4 worldPos = {ndcX, ndcY, 0.0f, 1.0f};
    glm_mat4_mulv(invMatrix, worldPos, worldPos);
    
    // Update gravity point with world space coordinates
    particle_system_set_gravity_point(world->particle_system, 
                                    worldPos[0] / worldPos[3], 
                                    worldPos[1] / worldPos[3]);

    // Update simulation
    particle_system_update(world->particle_system);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Render grid (temporarily disabled)
    // grid_render(&world->grid, (float*)view, (float*)projection);

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
    if (!world) return;

    // First cleanup HUD (ImGui)
    hud_cleanup(&world->hud);
    
    // Then cleanup particle system
    if (world->particle_system) {
        particle_system_cleanup(world->particle_system);
    }
    
    // Finally cleanup grid
    grid_cleanup(&world->grid);
}