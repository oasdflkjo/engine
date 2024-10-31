#include "world.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <math.h>
#include <GLFW/glfw3.h>

static void init_particle_positions(vec2* positions, int numParticles) {
    // Pre-calculate random values in bulk for better performance
    float* rand_vals = (float*)_mm_malloc(numParticles * 2 * sizeof(float), 16);
    for (int i = 0; i < numParticles * 2; i++) {
        rand_vals[i] = (float)rand();
    }

    __m128 twenty = _mm_set1_ps(20.0f);
    __m128 ten = _mm_set1_ps(10.0f);
    __m128 rand_max = _mm_set1_ps((float)RAND_MAX);
    
    // Process 4 particles (8 floats) at a time
    int aligned_count = (numParticles / 4) * 4;
    for (int i = 0; i < aligned_count; i += 4) {
        __m128 rand_vec1 = _mm_load_ps(&rand_vals[i * 2]);
        __m128 rand_vec2 = _mm_load_ps(&rand_vals[i * 2 + 4]);
        
        __m128 normalized1 = _mm_div_ps(rand_vec1, rand_max);
        __m128 normalized2 = _mm_div_ps(rand_vec2, rand_max);
        
        __m128 positioned1 = _mm_sub_ps(_mm_mul_ps(normalized1, twenty), ten);
        __m128 positioned2 = _mm_sub_ps(_mm_mul_ps(normalized2, twenty), ten);
        
        _mm_store_ps((float*)&positions[i], positioned1);
        _mm_store_ps((float*)&positions[i + 2], positioned2);
    }
    
    // Handle remaining particles
    for (int i = aligned_count; i < numParticles; i++) {
        positions[i][0] = ((float)rand_vals[i * 2] / RAND_MAX) * 20.0f - 10.0f;
        positions[i][1] = ((float)rand_vals[i * 2 + 1] / RAND_MAX) * 20.0f - 10.0f;
    }

    _mm_free(rand_vals);
}

static void simd_zero_velocities(vec2* velocities, int numParticles) {
    __m128 zero = _mm_setzero_ps();  // Create a vector of zeros
    
    // Process 2 vec2's (4 floats) at a time
    int aligned_count = (numParticles / 2) * 2;
    for (int i = 0; i < aligned_count; i += 2) {
        _mm_store_ps((float*)&velocities[i], zero);
    }
    
    // Handle remaining elements
    for (int i = aligned_count; i < numParticles; i++) {
        velocities[i][0] = 0.0f;
        velocities[i][1] = 0.0f;
    }
}

static void init_particle_velocities(vec2* velocities, int numParticles) {
    simd_zero_velocities(velocities, numParticles);
}

static void init_particle_buffers(World* world, vec2* positions, vec2* velocities) {
    glGenBuffers(1, &world->positionBuffer);
    glGenBuffers(1, &world->velocityBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world->positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, world->numParticles * sizeof(vec2), positions, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world->velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, world->numParticles * sizeof(vec2), velocities, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &world->particleVAO);
    glBindVertexArray(world->particleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world->positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
    glEnableVertexAttribArray(0);
}

static void init_particles(World* world) {
    world->numParticles = MAX_PARTICLES;
    
    vec2* positions = (vec2*)malloc(world->numParticles * sizeof(vec2));
    vec2* velocities = (vec2*)malloc(world->numParticles * sizeof(vec2));

    if (!positions || !velocities) {
        free(positions);
        free(velocities);
        return;
    }

    init_particle_positions(positions, world->numParticles);
    init_particle_velocities(velocities, world->numParticles);
    init_particle_buffers(world, positions, velocities);

    free(positions);
    free(velocities);
}

void world_init(World* world) {
    // Start with grid initialization
    grid_init(&world->grid, 10.0f, 1.0f);

    world->mousePos[0] = 0.0f;
    world->mousePos[1] = 0.0f;
    world->deltaTime = 0.0f;

    // Load all shader sources first and check for errors
    char* computeSource = read_shader_file("shaders/particle.comp");
    char* vertexSource = read_shader_file("shaders/particle.vert");
    char* fragmentSource = read_shader_file("shaders/particle.frag");

    if (!computeSource || !vertexSource || !fragmentSource) {
        free(computeSource);
        free(vertexSource);
        free(fragmentSource);
        fprintf(stderr, "Failed to load shader sources\n");
        return;
    }

    // Compile shaders
    unsigned int computeShader = compile_shader(computeSource, GL_COMPUTE_SHADER);
    unsigned int vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    world->computeProgram = glCreateProgram();
    glAttachShader(world->computeProgram, computeShader);
    glLinkProgram(world->computeProgram);
    check_program_linking(world->computeProgram, "Compute");
    glDeleteShader(computeShader);
    free(computeSource);

    world->renderProgram = glCreateProgram();
    glAttachShader(world->renderProgram, vertexShader);
    glAttachShader(world->renderProgram, fragmentShader);
    glLinkProgram(world->renderProgram);
    check_program_linking(world->renderProgram, "Render");
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    free(vertexSource);
    free(fragmentSource);

    init_particles(world);
}

void world_render(World* world, Camera* camera) {
    static float lastFrame = 0.0f;
    float currentFrame = glfwGetTime();
    world->deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // Update particle positions
    world_update_particles(world);

    // Clear the buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get matrices from camera
    mat4 view;
    mat4 projection;
    camera_get_view_matrix(camera, view);
    camera_get_projection_matrix(camera, projection);
    
    // Render grid
    grid_render(&world->grid, (float*)view, (float*)projection);

    // Render particles
    glUseProgram(world->renderProgram);
    glUniformMatrix4fv(glGetUniformLocation(world->renderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniformMatrix4fv(glGetUniformLocation(world->renderProgram, "projection"), 1, GL_FALSE, (float*)projection);

    glBindVertexArray(world->particleVAO);
    glDrawArrays(GL_POINTS, 0, world->numParticles);
}

void world_cleanup(World* world) {
    grid_cleanup(&world->grid);
    
    // Cleanup particle resources
    glDeleteVertexArrays(1, &world->particleVAO);
    glDeleteBuffers(1, &world->positionBuffer);
    glDeleteBuffers(1, &world->velocityBuffer);
    glDeleteProgram(world->computeProgram);
    glDeleteProgram(world->renderProgram);
}

void world_update_particles(World* world) { 
    glUseProgram(world->computeProgram);
    
    // Update uniforms
    glUniform1f(glGetUniformLocation(world->computeProgram, "delta_time"), world->deltaTime);
    glUniform2fv(glGetUniformLocation(world->computeProgram, "mouse_pos"), 1, world->mousePos);
    glUniform1i(glGetUniformLocation(world->computeProgram, "num_particles"), world->numParticles);

    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, world->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, world->velocityBuffer);

    // Dispatch compute shader
    int workGroupSize = 256;
    int numWorkGroups = (world->numParticles + workGroupSize - 1) / workGroupSize;
    glDispatchCompute(numWorkGroups, 1, 1);

    // Make sure compute shader finishes before rendering
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void world_set_mouse_pos(World* world, float x, float y) {
    world->mousePos[0] = x;
    world->mousePos[1] = y;
}