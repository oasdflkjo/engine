#include "particle_system.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2

#define MAX_PARTICLES 70000000

static inline uint32_t xorshift32(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void init_particle_positions(vec2 *positions, int numParticles)
{
    // Initialize multiple states for parallel random generation
    __m128i state = _mm_set_epi32(
        0xDEADBEEF, // Different prime numbers/seeds
        0xB00B1E55, // for each lane
        0xBADF00D5,
        0xCAFEBABE);

    // Change scale and offset to center around (0,0)
    // Using -1.0 to 1.0 range first, then scaling to desired size
    __m128 scale = _mm_set1_ps(2.0f / (float)UINT32_MAX); // Scale to -1 to 1 range
    __m128 world_scale = _mm_set1_ps(100.0f);              // Then scale to world coordinates (-100 to 100)

    int aligned_count = (numParticles / 4) * 4;
    for (int i = 0; i < aligned_count; i += 4)
    {
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

        _mm_store_ps((float *)&positions[i], xy0);
        _mm_store_ps((float *)&positions[i + 2], xy1);
    }

    // Handle remaining particles
    uint32_t scalar_state = 0xCAFEBABE;
    for (int i = aligned_count; i < numParticles; i++)
    {
        uint32_t rx = xorshift32(&scalar_state);
        uint32_t ry = xorshift32(&scalar_state);

        // Scale to -100 to 100 range centered at origin
        positions[i][0] = ((rx / (float)UINT32_MAX) * 2.0f - 1.0f) * 200.0f;
        positions[i][1] = ((ry / (float)UINT32_MAX) * 2.0f - 1.0f) * 200.0f;
    }
}

static void simd_zero_velocities(vec2 *velocities, int numParticles)
{
    __m128 zero = _mm_setzero_ps();

    int aligned_count = (numParticles / 2) * 2;
    for (int i = 0; i < aligned_count; i += 2)
    {
        _mm_store_ps((float *)&velocities[i], zero);
    }

    for (int i = aligned_count; i < numParticles; i++)
    {
        velocities[i][0] = 0.0f;
        velocities[i][1] = 0.0f;
    }
}

static void init_particle_buffers(ParticleSystem *ps, vec2 *positions, vec2 *velocities)
{
    // Generate buffers
    glGenBuffers(3, ps->positionBuffers);
    glGenBuffers(3, ps->velocityBuffers);

    // Initialize buffers
    for (int i = 0; i < 3; i++)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->positionBuffers[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), positions, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->velocityBuffers[i]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, ps->numParticles * sizeof(vec2), velocities, GL_DYNAMIC_DRAW);
    }

    // Initialize VAO
    glGenVertexArrays(1, &ps->particleVAO);
    glBindVertexArray(ps->particleVAO);

    // Initialize indices first
    ps->computeIndex = 0;
    ps->renderIndex = 2;  // Start rendering from buffer 2
    ps->nextIndex = 1;

    // Set up VAO with the initial render buffer
    glBindBuffer(GL_ARRAY_BUFFER, ps->positionBuffers[ps->renderIndex]);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void *)0);
    glEnableVertexAttribArray(0);

    // Initialize fences
    for (int i = 0; i < 3; i++)
    {
        ps->fences[i] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    }
}

void particle_system_init(ParticleSystem *ps)
{
    ps->numParticles = MAX_PARTICLES;
    ps->count = ps->numParticles; // Set count to match actual number of particles
    ps->mousePos[0] = 0.0f;
    ps->mousePos[1] = 0.0f;
    ps->deltaTime = 0.0f;

    // Initialize shaders
    char *computeSource = read_shader_file("shaders/particle.comp");
    char *vertexSource = read_shader_file("shaders/particle.vert");
    char *fragmentSource = read_shader_file("shaders/particle.frag");

    if (!computeSource || !vertexSource || !fragmentSource)
    {
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
    vec2 *positions = (vec2 *)malloc(ps->numParticles * sizeof(vec2));
    vec2 *velocities = (vec2 *)malloc(ps->numParticles * sizeof(vec2));

    if (!positions || !velocities)
    {
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

void particle_system_update(ParticleSystem *ps)
{
    glUseProgram(ps->computeProgram);

    // Set uniforms
    glUniform1f(glGetUniformLocation(ps->computeProgram, "deltaTime"), ps->deltaTime);
    glUniform2fv(glGetUniformLocation(ps->computeProgram, "mousePos"), 1, ps->mousePos);
    glUniform1i(glGetUniformLocation(ps->computeProgram, "particleCount"), ps->numParticles);

    // Wait for compute buffer to be ready
    if (ps->fences[ps->computeIndex])
    {
        glClientWaitSync(ps->fences[ps->computeIndex], GL_SYNC_FLUSH_COMMANDS_BIT, 16666667);
        glDeleteSync(ps->fences[ps->computeIndex]);
        ps->fences[ps->computeIndex] = 0;
    }

    // Read from current buffer (input)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffers[ps->computeIndex]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->velocityBuffers[ps->computeIndex]);

    // Write to next buffer (output)
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ps->positionBuffers[ps->nextIndex]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ps->velocityBuffers[ps->nextIndex]);

    // Dispatch compute shader
    int workGroupSize = 1024;
    int numWorkGroups = (ps->numParticles + workGroupSize - 1) / workGroupSize;
    glDispatchCompute(numWorkGroups, 1, 1);

    // Ensure compute shader has finished
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    
    // Place fence for the newly computed buffer
    ps->fences[ps->nextIndex] = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    // Cycle the buffers:
    // Current input (computeIndex) -> becomes render
    // Current output (nextIndex) -> becomes next input
    // Current render -> becomes next output
    GLuint temp = ps->renderIndex;
    ps->renderIndex = ps->computeIndex;  // Input becomes render
    ps->computeIndex = ps->nextIndex;    // Output becomes input
    ps->nextIndex = temp;                // Render becomes output
}

void particle_system_render(ParticleSystem *ps, mat4 view, mat4 projection)
{
    // Wait for render buffer to be ready
    if (ps->fences[ps->renderIndex])
    {
        glClientWaitSync(ps->fences[ps->renderIndex], GL_SYNC_FLUSH_COMMANDS_BIT, GL_TIMEOUT_IGNORED);
        glDeleteSync(ps->fences[ps->renderIndex]);
        ps->fences[ps->renderIndex] = 0;
    }

    glUseProgram(ps->renderProgram);
    glUniformMatrix4fv(glGetUniformLocation(ps->renderProgram, "view"), 1, GL_FALSE, (float *)view);
    glUniformMatrix4fv(glGetUniformLocation(ps->renderProgram, "projection"), 1, GL_FALSE, (float *)projection);

    // Bind the render buffer for the vertex shader
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffers[ps->renderIndex]);

    // Draw particles
    glBindVertexArray(ps->particleVAO);
    glDrawArrays(GL_POINTS, 0, ps->numParticles);
}

void particle_system_cleanup(ParticleSystem *ps)
{
    glDeleteVertexArrays(1, &ps->particleVAO);
    glDeleteBuffers(3, ps->positionBuffers);
    glDeleteBuffers(3, ps->velocityBuffers);

    for (int i = 0; i < 3; i++)
    {
        if (ps->fences[i])
        {
            glDeleteSync(ps->fences[i]);
        }
    }

    glDeleteProgram(ps->computeProgram);
    glDeleteProgram(ps->renderProgram);
}

void particle_system_set_mouse_pos(ParticleSystem *ps, float x, float y)
{
    ps->mousePos[0] = x;
    ps->mousePos[1] = y;
}