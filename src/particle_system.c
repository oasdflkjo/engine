#include "particle_system.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

#define MAX_PARTICLES 5000000

static void init_particle_positions(vec2* positions, int numParticles) {
    // Pre-calculate random values in bulk for better performance
    float* rand_vals = (float*)_mm_malloc(numParticles * 2 * sizeof(float), 16);
    for (int i = 0; i < numParticles * 2; i++) {
        rand_vals[i] = (float)rand();
    }

    __m128 forty = _mm_set1_ps(40.0f);
    __m128 twenty = _mm_set1_ps(20.0f);
    __m128 rand_max = _mm_set1_ps((float)RAND_MAX);
    
    int aligned_count = (numParticles / 4) * 4;
    for (int i = 0; i < aligned_count; i += 4) {
        __m128 rand_vec1 = _mm_load_ps(&rand_vals[i * 2]);
        __m128 rand_vec2 = _mm_load_ps(&rand_vals[i * 2 + 4]);
        
        __m128 normalized1 = _mm_div_ps(rand_vec1, rand_max);
        __m128 normalized2 = _mm_div_ps(rand_vec2, rand_max);
        
        __m128 positioned1 = _mm_sub_ps(_mm_mul_ps(normalized1, forty), twenty);
        __m128 positioned2 = _mm_sub_ps(_mm_mul_ps(normalized2, forty), twenty);
        
        _mm_store_ps((float*)&positions[i], positioned1);
        _mm_store_ps((float*)&positions[i + 2], positioned2);
    }
    
    for (int i = aligned_count; i < numParticles; i++) {
        positions[i][0] = ((float)rand_vals[i * 2] / RAND_MAX) * 40.0f - 20.0f;
        positions[i][1] = ((float)rand_vals[i * 2 + 1] / RAND_MAX) * 40.0f - 20.0f;
    }

    _mm_free(rand_vals);
}

static void simd_zero_velocities(vec2* velocities, int numParticles) {
    __m128 zero = _mm_setzero_ps();
    
    int aligned_count = (numParticles / 2) * 2;
    for (int i = 0; i < aligned_count; i += 2) {
        _mm_store_ps((float*)&velocities[i], zero);
    }
    
    for (int i = aligned_count; i < numParticles; i++) {
        velocities[i][0] = 0.0f;
        velocities[i][1] = 0.0f;
    }
}

static void init_particle_buffers(ParticleSystem* ps, vec2* positions, vec2* velocities) {
    glGenBuffers(1, &ps->positionBuffer);
    glGenBuffers(1, &ps->velocityBuffer);
    glGenBuffers(1, &ps->velocityMagBuffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), positions, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), velocities, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->velocityMagBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    glGenVertexArrays(1, &ps->particleVAO);
    glBindVertexArray(ps->particleVAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, ps->positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, ps->velocityMagBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
}

void particle_system_init(ParticleSystem* ps) {
    ps->numParticles = MAX_PARTICLES;
    ps->count = ps->numParticles;  // Set count to match actual number of particles
    ps->mousePos[0] = 0.0f;
    ps->mousePos[1] = 0.0f;
    ps->deltaTime = 0.0f;
    
    // Initialize shaders
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

    // Compile and link shaders
    unsigned int computeShader = compile_shader(computeSource, GL_COMPUTE_SHADER);
    unsigned int vertexShader = compile_shader(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compile_shader(fragmentSource, GL_FRAGMENT_SHADER);

    ps->computeProgram = glCreateProgram();
    glAttachShader(ps->computeProgram, computeShader);
    glLinkProgram(ps->computeProgram);
    check_program_linking(ps->computeProgram, "Compute");
    glDeleteShader(computeShader);

    ps->renderProgram = glCreateProgram();
    glAttachShader(ps->renderProgram, vertexShader);
    glAttachShader(ps->renderProgram, fragmentShader);
    glLinkProgram(ps->renderProgram);
    check_program_linking(ps->renderProgram, "Render");
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    free(computeSource);
    free(vertexSource);
    free(fragmentSource);

    // Initialize particle data
    vec2* positions = (vec2*)malloc(ps->numParticles * sizeof(vec2));
    vec2* velocities = (vec2*)malloc(ps->numParticles * sizeof(vec2));

    if (!positions || !velocities) {
        free(positions);
        free(velocities);
        return;
    }

    init_particle_positions(positions, ps->numParticles);
    simd_zero_velocities(velocities, ps->numParticles);
    init_particle_buffers(ps, positions, velocities);

    free(positions);
    free(velocities);
}

void particle_system_update(ParticleSystem* ps) {
    glUseProgram(ps->computeProgram);
    
    glUniform1f(glGetUniformLocation(ps->computeProgram, "delta_time"), ps->deltaTime);
    glUniform2fv(glGetUniformLocation(ps->computeProgram, "mouse_pos"), 1, ps->mousePos);
    glUniform1i(glGetUniformLocation(ps->computeProgram, "num_particles"), ps->numParticles);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->velocityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ps->velocityMagBuffer);

    int workGroupSize = 256;
    int numWorkGroups = (ps->numParticles + workGroupSize - 1) / workGroupSize;
    glDispatchCompute(numWorkGroups, 1, 1);

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection) {
    glUseProgram(ps->renderProgram);
    glUniformMatrix4fv(glGetUniformLocation(ps->renderProgram, "view"), 1, GL_FALSE, (float*)view);
    glUniformMatrix4fv(glGetUniformLocation(ps->renderProgram, "projection"), 1, GL_FALSE, (float*)projection);

    glBindVertexArray(ps->particleVAO);
    glDrawArrays(GL_POINTS, 0, ps->numParticles);
}

void particle_system_cleanup(ParticleSystem* ps) {
    glDeleteVertexArrays(1, &ps->particleVAO);
    glDeleteBuffers(1, &ps->positionBuffer);
    glDeleteBuffers(1, &ps->velocityBuffer);
    glDeleteBuffers(1, &ps->velocityMagBuffer);
    glDeleteProgram(ps->computeProgram);
    glDeleteProgram(ps->renderProgram);
}

void particle_system_set_mouse_pos(ParticleSystem* ps, float x, float y) {
    ps->mousePos[0] = x;
    ps->mousePos[1] = y;
}