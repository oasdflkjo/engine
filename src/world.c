#include "world.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GLFW/glfw3.h>

#define GPU_QUERY_RING_SIZE 8

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
    world->hud.computeMs = 0.0f;
    world->hud.drawMs = 0.0f;

    // Initialize GPU timer queries (double-buffered to avoid stalls)
    glGenQueries(GPU_QUERY_RING_SIZE, world->computeQueries);
    glGenQueries(GPU_QUERY_RING_SIZE, world->drawQueries);
    world->queryIndex = 0;
    world->framesRendered = 0;
}

void world_render(World* world) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    float deltaTime = (lastFrame > 0.0f) ? (currentFrame - lastFrame) : (1.0f / 60.0f);
    lastFrame = currentFrame;
    
    int writeIndex = world->queryIndex;
    int readIndex = (writeIndex + 1) % GPU_QUERY_RING_SIZE;
    if (world->framesRendered >= GPU_QUERY_RING_SIZE) {
        GLuint available = 0;
        glGetQueryObjectuiv(world->computeQueries[readIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(world->computeQueries[readIndex], GL_QUERY_RESULT, &ns);
            world->hud.computeMs = (float)ns / 1000000.0f;
        }

        available = 0;
        glGetQueryObjectuiv(world->drawQueries[readIndex], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 ns = 0;
            glGetQueryObjectui64v(world->drawQueries[readIndex], GL_QUERY_RESULT, &ns);
            world->hud.drawMs = (float)ns / 1000000.0f;
        }
    }

    world->particle_system->deltaTime = deltaTime;
    particle_system_set_viewport_size(world->particle_system, world->camera.width, world->camera.height);
    float aspect = (float)world->camera.width / (float)world->camera.height;
    float halfHeight = world->camera.position[2];
    float halfWidth = aspect * halfHeight;
    float pixelsPerWorld = (float)world->camera.height / (2.0f * halfHeight);
    float minX = world->camera.target[0] - halfWidth;
    float maxX = world->camera.target[0] + halfWidth;
    float minY = world->camera.target[1] - halfHeight;
    float maxY = world->camera.target[1] + halfHeight;
    particle_system_set_pixels_per_world(world->particle_system, pixelsPerWorld);
    particle_system_set_view_bounds(world->particle_system, minX, minY, maxX, maxY);

    particle_system_set_gravity_point(world->particle_system, 
                                    world->camera.target[0], 
                                    world->camera.target[1]);

    // Update simulation
    glBeginQuery(GL_TIME_ELAPSED, world->computeQueries[writeIndex]);
    particle_system_update(world->particle_system);
    glEndQuery(GL_TIME_ELAPSED);

    // Clear buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(&world->camera, view);
    camera_get_projection_matrix(&world->camera, projection);
    
    // Render grid
    if (world->hud.showGrid) {
        grid_render(&world->grid, (float*)view, (float*)projection);
    }

    // Render particles
    glBeginQuery(GL_TIME_ELAPSED, world->drawQueries[writeIndex]);
    particle_system_render(world->particle_system, view, projection);
    glEndQuery(GL_TIME_ELAPSED);
    
    // Calculate FPS and frame time
    static float fps = 0.0f;
    static float frameTime = 0.0f;
    static float fpsUpdateTimer = 0.0f;
    static int fpsFrameCount = 0;
    
    fpsUpdateTimer += deltaTime;
    fpsFrameCount++;
    if (fpsUpdateTimer >= 0.25f) {
        float averageDeltaTime = fpsUpdateTimer / (float)fpsFrameCount;
        fps = (averageDeltaTime > 0.0f) ? (1.0f / averageDeltaTime) : 0.0f;
        frameTime = averageDeltaTime * 1000.0f;
        fpsUpdateTimer = 0.0f;
        fpsFrameCount = 0;
    }
    
    // Update HUD stats
    hud_update_stats(&world->hud, fps, world->particle_system->count, frameTime, deltaTime, world->hud.computeMs, world->hud.drawMs);
    
    // Render HUD
    hud_render(&world->hud);

    world->queryIndex = (world->queryIndex + 1) % GPU_QUERY_RING_SIZE;
    world->framesRendered++;
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

    glDeleteQueries(GPU_QUERY_RING_SIZE, world->computeQueries);
    glDeleteQueries(GPU_QUERY_RING_SIZE, world->drawQueries);
}
