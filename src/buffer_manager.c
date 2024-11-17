#include "buffer_manager.h"
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <stdlib.h>

static inline uint32_t xorshift32(uint32_t* state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

void init_particle_positions(vec2* positions, int numParticles) {
    __m128i state = _mm_set_epi32(0xDEADBEEF, 0xB00B1E55, 0xBADF00D5, 0xCAFEBABE);
    __m128 scale = _mm_set1_ps(2.0f / (float)UINT32_MAX);
    __m128 world_scale = _mm_set1_ps(500.0f);
    
    for (int i = 0; i < numParticles; i += 4) {
        // Generate random numbers
        __m128i rx = state;
        rx = _mm_xor_si128(rx, _mm_slli_epi32(rx, 13));
        rx = _mm_xor_si128(rx, _mm_srli_epi32(rx, 17));
        rx = _mm_xor_si128(rx, _mm_slli_epi32(rx, 5));
        state = rx;
        
        __m128i ry = rx;
        ry = _mm_xor_si128(ry, _mm_slli_epi32(ry, 13));
        ry = _mm_xor_si128(ry, _mm_srli_epi32(ry, 17));
        ry = _mm_xor_si128(ry, _mm_slli_epi32(ry, 5));
        
        // Convert and scale
        __m128 fx = _mm_mul_ps(_mm_mul_ps(_mm_cvtepi32_ps(rx), scale), world_scale);
        __m128 fy = _mm_mul_ps(_mm_mul_ps(_mm_cvtepi32_ps(ry), scale), world_scale);
        
        __m128 xy0 = _mm_unpacklo_ps(fx, fy);
        __m128 xy1 = _mm_unpackhi_ps(fx, fy);
        
        int remaining = numParticles - i;
        if (remaining >= 4) {
            _mm_store_ps((float*)&positions[i], xy0);
            _mm_store_ps((float*)&positions[i + 2], xy1);
        } else {
            float temp[4];
            _mm_store_ps(temp, xy0);
            for (int j = 0; j < remaining && j < 2; j++) {
                positions[i + j][0] = temp[j*2];
                positions[i + j][1] = temp[j*2 + 1];
            }
            if (remaining > 2) {
                _mm_store_ps(temp, xy1);
                for (int j = 0; j < remaining - 2; j++) {
                    positions[i + j + 2][0] = temp[j*2];
                    positions[i + j + 2][1] = temp[j*2 + 1];
                }
            }
        }
    }
}

void zero_particle_velocities(vec2* velocities, int numParticles) {
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

ParticleBuffers create_particle_buffers(int numParticles, vec2* positions, vec2* velocities) {
    ParticleBuffers buffers = {0};
    
    // Create all buffers at once
    GLuint buffer_ids[3];
    glGenBuffers(3, buffer_ids);
    buffers.positionBuffer = buffer_ids[0];
    buffers.velocityBuffer = buffer_ids[1];
    buffers.velocityMagBuffer = buffer_ids[2];

    // Initialize buffers with data
    struct {
        GLuint buffer;
        void* data;
        size_t size;
    } buffer_data[] = {
        {buffers.positionBuffer, positions, numParticles * sizeof(vec2)},
        {buffers.velocityBuffer, velocities, numParticles * sizeof(vec2)},
        {buffers.velocityMagBuffer, NULL, numParticles * sizeof(float)}
    };

    for (int i = 0; i < 3; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffer_data[i].buffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, buffer_data[i].size, 
                    buffer_data[i].data, GL_DYNAMIC_DRAW);
    }

    // Setup VAO for instanced rendering
    glGenVertexArrays(1, &buffers.particleVAO);
    glBindVertexArray(buffers.particleVAO);
    
    // Position attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffers.positionBuffer);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribDivisor(0, 1);

    // Velocity magnitude attribute
    glBindBuffer(GL_ARRAY_BUFFER, buffers.velocityMagBuffer);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    return buffers;
}

void destroy_particle_buffers(ParticleBuffers* buffers) {
    GLuint buffer_ids[] = {
        buffers->positionBuffer,
        buffers->velocityBuffer,
        buffers->velocityMagBuffer
    };
    glDeleteBuffers(3, buffer_ids);
    glDeleteVertexArrays(1, &buffers->particleVAO);
} 