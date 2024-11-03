#include "particle_system.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2

#define MAX_PARTICLES 15000000

static inline uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void init_particle_positions(vec2* positions, int numParticles) {
    // Initialize multiple states for parallel random generation
    __m128i state = _mm_set_epi32(
        0xDEADBEEF,  // Different prime numbers/seeds
        0xB00B1E55,  // for each lane
        0xBADF00D5,
        0xCAFEBABE
    );
    
    // Change scale and offset to center around (0,0)
    // Using -1.0 to 1.0 range first, then scaling to desired size
    __m128 scale = _mm_set1_ps(2.0f / (float)UINT32_MAX);  // Scale to -1 to 1 range
    __m128 world_scale = _mm_set1_ps(20.0f);  // Then scale to world coordinates
    
    int aligned_count = (numParticles / 4) * 4;
    for (int i = 0; i < aligned_count; i += 4) {
        // Generate random numbers using XOR-shift (same as before)
        __m128i rx = state;
        rx = _mm_xor_si128(rx, _mm_slli_epi32(rx, 13));
        rx = _mm_xor_si128(rx, _mm_srli_epi32(rx, 17));
        rx = _mm_xor_si128(rx, _mm_slli_epi32(rx, 5));
        state = rx;
        
        __m128i ry = rx;
        ry = _mm_xor_si128(ry, _mm_slli_epi32(ry, 13));
        ry = _mm_xor_si128(ry, _mm_srli_epi32(ry, 17));
        ry = _mm_xor_si128(ry, _mm_slli_epi32(ry, 5));
        
        // Convert to floats and scale to -1 to 1 range
        __m128 fx = _mm_mul_ps(_mm_cvtepi32_ps(rx), scale);
        __m128 fy = _mm_mul_ps(_mm_cvtepi32_ps(ry), scale);
        
        // Scale to world coordinates (-20 to 20)
        fx = _mm_mul_ps(fx, world_scale);
        fy = _mm_mul_ps(fy, world_scale);
        
        // Store interleaved x,y coordinates
        __m128 xy0 = _mm_unpacklo_ps(fx, fy);
        __m128 xy1 = _mm_unpackhi_ps(fx, fy);
        
        _mm_store_ps((float*)&positions[i], xy0);
        _mm_store_ps((float*)&positions[i + 2], xy1);
    }
    
    // Handle remaining particles
    uint32_t scalar_state = 0xCAFEBABE;
    for (int i = aligned_count; i < numParticles; i++) {
        uint32_t rx = xorshift32(&scalar_state);
        uint32_t ry = xorshift32(&scalar_state);
        
        // Scale to -20 to 20 range centered at origin
        positions[i][0] = ((rx / (float)UINT32_MAX) * 2.0f - 1.0f) * 20.0f;
        positions[i][1] = ((ry / (float)UINT32_MAX) * 2.0f - 1.0f) * 20.0f;
    }
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

    // Remove glBufferStorage and use simpler setup
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->positionBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), positions, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->velocityBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), velocities, GL_DYNAMIC_DRAW);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->velocityMagBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(float), NULL, GL_DYNAMIC_DRAW);

    // Setup VAO
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

    // Cache uniform locations
    ps->deltaTimeLocation = glGetUniformLocation(ps->computeProgram, "delta_time");
    ps->mousePosLocation = glGetUniformLocation(ps->computeProgram, "mouse_pos");
    ps->numParticlesLocation = glGetUniformLocation(ps->computeProgram, "num_particles");
    ps->particleOffsetLocation = glGetUniformLocation(ps->computeProgram, "particle_offset");
    ps->batchSizeLocation = glGetUniformLocation(ps->computeProgram, "batch_size");
}

void particle_system_update(ParticleSystem* ps) {
    const int WORK_GROUP_SIZE = 32;  // Must match shader local_size_x/y
    const int PARTICLES_PER_GROUP = WORK_GROUP_SIZE * WORK_GROUP_SIZE;
    
    glUseProgram(ps->computeProgram);
    
    // Set uniforms once
    glUniform1f(ps->deltaTimeLocation, ps->deltaTime);
    glUniform2fv(ps->mousePosLocation, 1, ps->mousePos);
    glUniform1i(ps->numParticlesLocation, ps->numParticles);
    
    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->velocityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ps->velocityMagBuffer);

    // Calculate total number of work groups needed
    int total_groups = (ps->numParticles + PARTICLES_PER_GROUP - 1) / PARTICLES_PER_GROUP;
    
    // Calculate grid dimensions
    int groups_x = (int)ceil(sqrt((double)total_groups));
    int groups_y = (total_groups + groups_x - 1) / groups_x;
    
    // Single dispatch for all particles
    glUniform1i(ps->particleOffsetLocation, 0);
    glUniform1i(ps->batchSizeLocation, ps->numParticles);
    
    glDispatchCompute(groups_x, groups_y, 1);
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