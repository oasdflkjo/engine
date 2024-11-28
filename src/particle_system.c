#include "particle_system.h"
#include "buffer_manager.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>  // Include for sqrt function

#define MAX_PARTICLES 60000000
#define NUM_BINS 1024  // Define NUM_BINS here

ParticleSystem* particle_system_create(void) {
    ParticleSystem* ps = malloc(sizeof(ParticleSystem));
    if (!ps) {
        return NULL;
    }
    
    // Initialize default values
    ps->numParticles = MAX_PARTICLES;
    ps->count = ps->numParticles;
    ps->deltaTime = 0.0f;
    ps->timeScale = 0.1f;
    ps->attractionStrength = 1.0f;
    ps->gravityPoint[0] = 0.0f;
    ps->gravityPoint[1] = 0.0f;
    
    // Initialize physics parameters with default values
    ps->minDistance = 0.0001f;
    ps->forceScale = 150.0f;
    ps->maxForce = 200.0f;
    ps->terminalVelocity = 100.0f;
    ps->damping = 0.9f;
    ps->mouseForceRadius = 5.0f;
    ps->mouseForceStrength = 1.0f;
    
    return ps;
}

static bool init_shaders(ParticleSystem* ps) {
    const char* compute_files[] = {"shaders/particle.comp"};
    GLenum compute_types[] = {GL_COMPUTE_SHADER};
    ps->computeProgram = shader_program_create(compute_files, compute_types, 1);
    if (!ps->computeProgram) {
        return false;
    }

    const char* render_files[] = {
        "shaders/particle.vert",
        "shaders/particle.frag"
    };
    GLenum render_types[] = {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };
    ps->renderProgram = shader_program_create(render_files, render_types, 2);
    if (!ps->renderProgram) {
        shader_program_destroy(ps->computeProgram);
        return false;
    }

    // Cache uniform locations for compute program
    const char* compute_uniforms[] = {
        "delta_time",
        "gravity_point",
        "num_particles",
        "particle_offset",
        "batch_size",
        "min_distance",
        "force_scale",
        "max_force",
        "terminal_velocity",
        "damping",
        "gravity_radius",
        "gravity_strength",
        "attraction_strength",
        "time_scale"
    };
    shader_program_cache_uniforms(ps->computeProgram, compute_uniforms, sizeof(compute_uniforms)/sizeof(compute_uniforms[0]));

    return true;
}

static void init_uniform_locations(ParticleSystem* ps) {
    struct UniformInfo {
        const char* name;
        GLint* location;
    } uniforms[] = {
        {"delta_time", &ps->deltaTimeLocation},
        {"gravity_point", &ps->gravityPointLocation},
        {"num_particles", &ps->numParticlesLocation},
        {"particle_offset", &ps->particleOffsetLocation},
        {"batch_size", &ps->batchSizeLocation},
        {"min_distance", &ps->minDistanceLocation},
        {"force_scale", &ps->forceScaleLocation},
        {"max_force", &ps->maxForceLocation},
        {"terminal_velocity", &ps->terminalVelocityLocation},
        {"damping", &ps->dampingLocation},
        {"gravity_radius", &ps->mouseForceRadiusLocation},
        {"gravity_strength", &ps->mouseForceStrengthLocation},
        {"attraction_strength", &ps->attractionStrengthLocation},
        {"time_scale", &ps->timeScaleLocation}
    };

    for (size_t i = 0; i < sizeof(uniforms)/sizeof(uniforms[0]); i++) {
        *uniforms[i].location = shader_program_get_uniform(ps->computeProgram, uniforms[i].name);
    }
}

static bool init_binning(ParticleSystem* ps) {
    const char* binning_files[] = {"shaders/binning.comp"};
    GLenum binning_types[] = {GL_COMPUTE_SHADER};
    ps->binningProgram = shader_program_create(binning_files, binning_types, 1);
    if (!ps->binningProgram) {
        return false;
    }

    // Calculate optimal bin size based on screen size and particle count
    ps->screenWidth = 1920;  // Get actual screen width
    ps->screenHeight = 1080; // Get actual screen height
    float particleDensity = ps->numParticles / (float)(ps->screenWidth * ps->screenHeight);
    ps->optimalBinSize = sqrt(1.0f / particleDensity) * 2.0f;  // Adjust this factor
    
    ps->numBinsX = (uint32_t)(ps->screenWidth / ps->optimalBinSize);
    ps->numBinsY = (uint32_t)(ps->screenHeight / ps->optimalBinSize);
    uint32_t totalBins = ps->numBinsX * ps->numBinsY;

    // Create buffer for bin counts
    glGenBuffers(1, &ps->binCountBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->binCountBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(uint32_t) * totalBins, NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->binCountBuffer);

    // Cache uniform locations
    shader_program_use(ps->binningProgram);
    ps->binSizeLocation = shader_program_get_uniform(ps->binningProgram, "binSize");
    ps->screenSizeLocation = shader_program_get_uniform(ps->binningProgram, "screenSize");
    ps->numBinsXLocation = shader_program_get_uniform(ps->binningProgram, "numBinsX");
    ps->numBinsYLocation = shader_program_get_uniform(ps->binningProgram, "numBinsY");

    return true;
}

void particle_system_init(ParticleSystem* ps) {
    if (!init_shaders(ps)) {
        return;
    }

    // Initialize binning after shaders
    if (!init_binning(ps)) {
        shader_program_destroy(ps->computeProgram);
        shader_program_destroy(ps->renderProgram);
        return;
    }

    // Initialize particle data
    vec2* positions = (vec2*)malloc(ps->numParticles * sizeof(vec2));
    vec2* velocities = (vec2*)malloc(ps->numParticles * sizeof(vec2));

    if (!positions || !velocities) {
        free(positions);
        free(velocities);
        return;
    }

    // Use buffer manager functions
    init_particle_positions(positions, ps->numParticles);
    zero_particle_velocities(velocities, ps->numParticles);
    
    // Create buffers using buffer manager
    ParticleBuffers buffers = create_particle_buffers(ps->numParticles, positions, velocities);
    ps->positionBuffer = buffers.positionBuffer;
    ps->velocityBuffer = buffers.velocityBuffer;
    ps->velocityMagBuffer = buffers.velocityMagBuffer;
    ps->particleVAO = buffers.particleVAO;

    free(positions);
    free(velocities);

    init_uniform_locations(ps);
}

struct UniformUpdate {
    GLint location;
    GLenum type;
    union {
        float f;
        int i;
        float v2[2];
    } value;
};

#define UNIFORM_UPDATE(loc, type, val) { \
    struct UniformUpdate update = {loc, type, {.f = (float)(val)}}; \
    switch (type) { \
        case GL_FLOAT: glUniform1f(update.location, update.value.f); break; \
        case GL_INT: glUniform1i(update.location, (int)update.value.f); break; \
        case GL_FLOAT_VEC2: glUniform2fv(update.location, 1, (float*)&update.value); break; \
    } \
}

static void update_uniforms(ParticleSystem* ps) {
    shader_program_use(ps->computeProgram);
    shader_program_set_float(ps->computeProgram, "delta_time", ps->deltaTime);
    shader_program_set_int(ps->computeProgram, "num_particles", ps->numParticles);
    shader_program_set_float(ps->computeProgram, "min_distance", ps->minDistance);
    shader_program_set_float(ps->computeProgram, "force_scale", ps->forceScale);
    shader_program_set_float(ps->computeProgram, "max_force", ps->maxForce);
    shader_program_set_float(ps->computeProgram, "terminal_velocity", ps->terminalVelocity);
    shader_program_set_float(ps->computeProgram, "damping", ps->damping);
    shader_program_set_float(ps->computeProgram, "gravity_radius", ps->mouseForceRadius);
    shader_program_set_float(ps->computeProgram, "gravity_strength", ps->mouseForceStrength);
    shader_program_set_float(ps->computeProgram, "attraction_strength", ps->attractionStrength);
    shader_program_set_float(ps->computeProgram, "time_scale", ps->timeScale);
    shader_program_set_vec2(ps->computeProgram, "gravity_point", ps->gravityPoint[0], ps->gravityPoint[1]);
}

void particle_system_update(ParticleSystem* ps) {
    const int WORK_GROUP_SIZE = 256;  // Match shader local size
    
    shader_program_use(ps->computeProgram);
    update_uniforms(ps);
    
    // Bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->velocityBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ps->velocityMagBuffer);

    // Calculate dispatch dimensions
    int particles_per_group = WORK_GROUP_SIZE;
    int groups = (ps->numParticles + particles_per_group - 1) / particles_per_group;
    
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Clear bin counts before dispatching binning shader
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ps->binCountBuffer);
    uint32_t zero = 0;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

    // Dispatch binning compute shader
    shader_program_use(ps->binningProgram);
    glUniform1f(ps->binSizeLocation, ps->optimalBinSize);
    glUniform2f(ps->screenSizeLocation, (float)ps->screenWidth, (float)ps->screenHeight);
    glUniform1ui(ps->numBinsXLocation, ps->numBinsX);
    glUniform1ui(ps->numBinsYLocation, ps->numBinsY);

    // Bind position buffer for binning
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->positionBuffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ps->binCountBuffer);

    // Dispatch binning compute shader
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection) {
    shader_program_use(ps->renderProgram);
    shader_program_set_mat4(ps->renderProgram, "view", (float*)view);
    shader_program_set_mat4(ps->renderProgram, "projection", (float*)projection);

    glBindVertexArray(ps->particleVAO);
    glDrawArraysInstanced(GL_POINTS, 0, 1, ps->numParticles);
}

void particle_system_cleanup(ParticleSystem* ps) {
    if (!ps) return;

    // First destroy buffers
    ParticleBuffers buffers = {
        .positionBuffer = ps->positionBuffer,
        .velocityBuffer = ps->velocityBuffer,
        .velocityMagBuffer = ps->velocityMagBuffer,
        .particleVAO = ps->particleVAO
    };
    destroy_particle_buffers(&buffers);
    
    // Clean up binning resources
    if (ps->binCountBuffer) {
        glDeleteBuffers(1, &ps->binCountBuffer);
    }
    
    // Then destroy shader programs
    if (ps->computeProgram) {
        shader_program_destroy(ps->computeProgram);
        ps->computeProgram = NULL;
    }
    if (ps->renderProgram) {
        shader_program_destroy(ps->renderProgram);
        ps->renderProgram = NULL;
    }
    if (ps->binningProgram) {
        shader_program_destroy(ps->binningProgram);
        ps->binningProgram = NULL;
    }
}

// Simple parameter setters
void particle_system_set_gravity_point(ParticleSystem* ps, float x, float y) {
    ps->gravityPoint[0] = x;
    ps->gravityPoint[1] = y;
}

static void particle_system_set_float_param(ParticleSystem* ps, float value, float* param, const char* uniform_name) {
    *param = value;
    shader_program_use(ps->computeProgram);
    shader_program_set_float(ps->computeProgram, uniform_name, value);
}

void particle_system_set_force_scale(ParticleSystem* ps, float scale) {
    particle_system_set_float_param(ps, scale, &ps->forceScale, "force_scale");
}

void particle_system_set_damping(ParticleSystem* ps, float damping) {
    particle_system_set_float_param(ps, damping, &ps->damping, "damping");
}

void particle_system_set_terminal_velocity(ParticleSystem* ps, float velocity) {
    particle_system_set_float_param(ps, velocity, &ps->terminalVelocity, "terminal_velocity");
}

void particle_system_set_attraction_strength(ParticleSystem* ps, float strength) {
    ps->attractionStrength = strength;
}

void particle_system_set_time_scale(ParticleSystem* ps, float scale) {
    ps->timeScale = scale;
}

float particle_system_get_time_scale(ParticleSystem* ps) {
    return ps->timeScale;
}

float particle_system_get_attraction_strength(ParticleSystem* ps) {
    return ps->attractionStrength;
}

void particle_system_destroy(ParticleSystem* ps) {
    if (ps) {
        particle_system_cleanup(ps);
        free(ps);
    }
}

static void adjust_bin_size(ParticleSystem* ps) {
    // Adjust bin size based on particle density and frame time
    static float lastFrameTime = 0.0f;
    float frameTime = ps->deltaTime * 1000.0f;  // Convert to milliseconds
    
    // Adjust bin size if frame time is too high or too low
    if (frameTime > 16.7f) {  // Below 60 FPS
        ps->optimalBinSize *= 1.1f;  // Increase bin size to reduce workload
    } else if (frameTime < 14.0f && ps->optimalBinSize > 2.0f) {  // Well above 60 FPS
        ps->optimalBinSize *= 0.9f;  // Decrease bin size for better accuracy
    }
    
    // Update bin counts based on new bin size
    ps->numBinsX = (uint32_t)(ps->screenWidth / ps->optimalBinSize);
    ps->numBinsY = (uint32_t)(ps->screenHeight / ps->optimalBinSize);
    
    lastFrameTime = frameTime;
}

static void optimize_bin_dimensions(ParticleSystem* ps) {
    // Round to nearest power of 2 for more efficient indexing
    ps->numBinsX = 1u << (32 - __builtin_clz((unsigned int)ps->numBinsX - 1));
    ps->numBinsY = 1u << (32 - __builtin_clz((unsigned int)ps->numBinsY - 1));
    
    // Adjust bin size to match new dimensions
    ps->optimalBinSize = (float)ps->screenWidth / ps->numBinsX;
}