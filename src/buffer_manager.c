#include "buffer_manager.h"
#include <xmmintrin.h>  // SSE
#include <emmintrin.h>  // SSE2
#include <stdlib.h>
#include <stdint.h>

void init_particle_data(vec4* particleData, int numParticles) {
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
        
        float xy_temp[8];
        _mm_storeu_ps(&xy_temp[0], xy0);
        _mm_storeu_ps(&xy_temp[4], xy1);

        int remaining = numParticles - i;
        int batch = remaining < 4 ? remaining : 4;
        for (int j = 0; j < batch; j++) {
            particleData[i + j][0] = xy_temp[j * 2];
            particleData[i + j][1] = xy_temp[j * 2 + 1];
            particleData[i + j][2] = 0.0f;
            particleData[i + j][3] = 0.0f;
        }
    }
}

ParticleBuffers create_particle_buffers(int numParticles, vec4* particleData) {
    ParticleBuffers buffers = {0};
    
    glGenBuffers(1, &buffers.particleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.particleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(vec4), particleData, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &buffers.visibleParticleBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buffers.visibleParticleBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, numParticles * sizeof(vec4), NULL, GL_DYNAMIC_DRAW);

    glGenBuffers(1, &buffers.drawCommandBuffer);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffers.drawCommandBuffer);
    GLuint drawCmd[4] = {0u, 1u, 0u, 0u}; // count, instanceCount, first, baseInstance
    glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(drawCmd), drawCmd, GL_DYNAMIC_DRAW);

    // Setup VAO for direct per-particle rendering
    glGenVertexArrays(1, &buffers.particleVAO);
    glBindVertexArray(buffers.particleVAO);

    glBindBuffer(GL_ARRAY_BUFFER, buffers.visibleParticleBuffer);

    // Position: vec2 at offset 0 in vec4
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*)0);
    glEnableVertexAttribArray(0);

    // Velocity: vec2 at offset 8 bytes in vec4
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return buffers;
}

void destroy_particle_buffers(ParticleBuffers* buffers) {
    GLuint buffer_ids[] = {
        buffers->particleBuffer,
        buffers->visibleParticleBuffer,
        buffers->drawCommandBuffer
    };
    glDeleteBuffers(3, buffer_ids);
    glDeleteVertexArrays(1, &buffers->particleVAO);
}
