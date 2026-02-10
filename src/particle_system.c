#include "particle_system.h"
#include "buffer_manager.h"
#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_PARTICLES 100000000
#define DENSITY_CLEAR_GROUP_SIZE 16
#define PARTICLE_GROUP_SIZE 256

static bool recreate_density_texture(ParticleSystem* ps) {
    if (ps->viewportWidth <= 0 || ps->viewportHeight <= 0) {
        return false;
    }

    if (ps->densityTexture != 0) {
        glDeleteTextures(1, &ps->densityTexture);
        ps->densityTexture = 0;
    }

    glGenTextures(1, &ps->densityTexture);
    glBindTexture(GL_TEXTURE_2D, ps->densityTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, ps->viewportWidth, ps->viewportHeight);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

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
    ps->pixelsPerWorld = 1.0f;
    ps->particleRadiusWorld = 0.5f;
    ps->viewportWidth = 1;
    ps->viewportHeight = 1;
    ps->viewMin[0] = -500.0f;
    ps->viewMin[1] = -500.0f;
    ps->viewMax[0] = 500.0f;
    ps->viewMax[1] = 500.0f;
    ps->densityTexture = 0;
    ps->screenVAO = 0;
    
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

    const char* cull_files[] = {"shaders/particle_cull.comp"};
    GLenum cull_types[] = {GL_COMPUTE_SHADER};
    ps->cullProgram = shader_program_create(cull_files, cull_types, 1);
    if (!ps->cullProgram) {
        shader_program_destroy(ps->computeProgram);
        return false;
    }

    const char* density_clear_files[] = {"shaders/particle_density_clear.comp"};
    GLenum density_clear_types[] = {GL_COMPUTE_SHADER};
    ps->densityClearProgram = shader_program_create(density_clear_files, density_clear_types, 1);
    if (!ps->densityClearProgram) {
        shader_program_destroy(ps->cullProgram);
        shader_program_destroy(ps->computeProgram);
        return false;
    }

    const char* density_accum_files[] = {"shaders/particle_density_accumulate.comp"};
    GLenum density_accum_types[] = {GL_COMPUTE_SHADER};
    ps->densityAccumulateProgram = shader_program_create(density_accum_files, density_accum_types, 1);
    if (!ps->densityAccumulateProgram) {
        shader_program_destroy(ps->densityClearProgram);
        shader_program_destroy(ps->cullProgram);
        shader_program_destroy(ps->computeProgram);
        return false;
    }

    const char* render_files[] = {
        "shaders/particle_resolve.vert",
        "shaders/particle_resolve.frag"
    };
    GLenum render_types[] = {
        GL_VERTEX_SHADER,
        GL_FRAGMENT_SHADER
    };
    ps->renderProgram = shader_program_create(render_files, render_types, 2);
    if (!ps->renderProgram) {
        shader_program_destroy(ps->densityAccumulateProgram);
        shader_program_destroy(ps->densityClearProgram);
        shader_program_destroy(ps->cullProgram);
        shader_program_destroy(ps->computeProgram);
        return false;
    }

    // Cache uniform locations for compute program
    const char* compute_uniforms[] = {
        "delta_time",
        "gravity_point",
        "num_particles",
        "attraction_strength",
        "time_scale"
    };
    shader_program_cache_uniforms(ps->computeProgram, compute_uniforms, sizeof(compute_uniforms)/sizeof(compute_uniforms[0]));

    const char* density_clear_uniforms[] = {
        "viewport_size"
    };
    shader_program_cache_uniforms(ps->densityClearProgram, density_clear_uniforms, 1);

    const char* density_accum_uniforms[] = {
        "num_particles",
        "view_min",
        "view_max",
        "viewport_size"
    };
    shader_program_cache_uniforms(ps->densityAccumulateProgram, density_accum_uniforms, 4);

    const char* render_uniforms[] = {
        "density_tex",
        "pixels_per_world",
        "particle_radius_world"
    };
    shader_program_cache_uniforms(ps->renderProgram, render_uniforms, 3);

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
        {"attraction_strength", &ps->attractionStrengthLocation},
        {"time_scale", &ps->timeScaleLocation}
    };

    for (size_t i = 0; i < sizeof(uniforms)/sizeof(uniforms[0]); i++) {
        *uniforms[i].location = shader_program_get_uniform(ps->computeProgram, uniforms[i].name);
    }

    ps->densityClearViewportLocation = shader_program_get_uniform(ps->densityClearProgram, "viewport_size");
    ps->densityAccumNumParticlesLocation = shader_program_get_uniform(ps->densityAccumulateProgram, "num_particles");
    ps->densityAccumViewMinLocation = shader_program_get_uniform(ps->densityAccumulateProgram, "view_min");
    ps->densityAccumViewMaxLocation = shader_program_get_uniform(ps->densityAccumulateProgram, "view_max");
    ps->densityAccumViewportLocation = shader_program_get_uniform(ps->densityAccumulateProgram, "viewport_size");
    ps->renderDensityTexLocation = shader_program_get_uniform(ps->renderProgram, "density_tex");
    ps->renderPixelsPerWorldLocation = shader_program_get_uniform(ps->renderProgram, "pixels_per_world");
    ps->renderParticleRadiusWorldLocation = shader_program_get_uniform(ps->renderProgram, "particle_radius_world");
}

void particle_system_init(ParticleSystem* ps) {
    if (!init_shaders(ps)) {
        return;
    }

    // Initialize packed particle data: xy = position, zw = velocity
    vec4* particleData = (vec4*)malloc(ps->numParticles * sizeof(vec4));

    if (!particleData) {
        free(particleData);
        return;
    }

    init_particle_data(particleData, ps->numParticles);
    
    // Create buffers using buffer manager
    ParticleBuffers buffers = create_particle_buffers(ps->numParticles, particleData);
    ps->particleBuffer = buffers.particleBuffer;
    ps->visibleParticleBuffer = buffers.visibleParticleBuffer;
    ps->drawCommandBuffer = buffers.drawCommandBuffer;
    ps->particleVAO = buffers.particleVAO;

    free(particleData);

    glGenVertexArrays(1, &ps->screenVAO);
    recreate_density_texture(ps);

    init_uniform_locations(ps);
}

static void update_uniforms(ParticleSystem* ps) {
    shader_program_use(ps->computeProgram);
    if (ps->deltaTimeLocation != -1) glUniform1f(ps->deltaTimeLocation, ps->deltaTime);
    if (ps->numParticlesLocation != -1) glUniform1i(ps->numParticlesLocation, ps->numParticles);
    if (ps->attractionStrengthLocation != -1) glUniform1f(ps->attractionStrengthLocation, ps->attractionStrength);
    if (ps->timeScaleLocation != -1) glUniform1f(ps->timeScaleLocation, ps->timeScale);
    if (ps->gravityPointLocation != -1) glUniform2fv(ps->gravityPointLocation, 1, ps->gravityPoint);
}

void particle_system_update(ParticleSystem* ps) {
    shader_program_use(ps->computeProgram);
    update_uniforms(ps);
    
    // Single packed particle buffer
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->particleBuffer);

    // 1D dispatch avoids sqrt dispatch shaping and skipped particles
    int groups = (ps->numParticles + PARTICLE_GROUP_SIZE - 1) / PARTICLE_GROUP_SIZE;
    
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);

    // Clear density texture
    shader_program_use(ps->densityClearProgram);
    if (ps->densityClearViewportLocation != -1) {
        glUniform2i(ps->densityClearViewportLocation, ps->viewportWidth, ps->viewportHeight);
    }
    glBindImageTexture(0, ps->densityTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32UI);
    int clearGroupsX = (ps->viewportWidth + DENSITY_CLEAR_GROUP_SIZE - 1) / DENSITY_CLEAR_GROUP_SIZE;
    int clearGroupsY = (ps->viewportHeight + DENSITY_CLEAR_GROUP_SIZE - 1) / DENSITY_CLEAR_GROUP_SIZE;
    glDispatchCompute(clearGroupsX, clearGroupsY, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    // Accumulate visible particles into density texture
    shader_program_use(ps->densityAccumulateProgram);
    if (ps->densityAccumNumParticlesLocation != -1) glUniform1i(ps->densityAccumNumParticlesLocation, ps->numParticles);
    if (ps->densityAccumViewMinLocation != -1) glUniform2fv(ps->densityAccumViewMinLocation, 1, ps->viewMin);
    if (ps->densityAccumViewMaxLocation != -1) glUniform2fv(ps->densityAccumViewMaxLocation, 1, ps->viewMax);
    if (ps->densityAccumViewportLocation != -1) glUniform2i(ps->densityAccumViewportLocation, ps->viewportWidth, ps->viewportHeight);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ps->particleBuffer);
    glBindImageTexture(0, ps->densityTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void particle_system_render(ParticleSystem* ps, mat4 view, mat4 projection) {
    (void)view;
    (void)projection;
    shader_program_use(ps->renderProgram);
    if (ps->renderDensityTexLocation != -1) glUniform1i(ps->renderDensityTexLocation, 0);
    if (ps->renderPixelsPerWorldLocation != -1) glUniform1f(ps->renderPixelsPerWorldLocation, ps->pixelsPerWorld);
    if (ps->renderParticleRadiusWorldLocation != -1) glUniform1f(ps->renderParticleRadiusWorldLocation, ps->particleRadiusWorld);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ps->densityTexture);
    glBindVertexArray(ps->screenVAO);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisable(GL_BLEND);
}

void particle_system_cleanup(ParticleSystem* ps) {
    if (!ps) return;

    // First destroy buffers
    ParticleBuffers buffers = {
        .particleBuffer = ps->particleBuffer,
        .visibleParticleBuffer = ps->visibleParticleBuffer,
        .drawCommandBuffer = ps->drawCommandBuffer,
        .particleVAO = ps->particleVAO
    };
    destroy_particle_buffers(&buffers);
    
    // Then destroy shader programs
    if (ps->computeProgram) {
        shader_program_destroy(ps->computeProgram);
        ps->computeProgram = NULL;
    }
    if (ps->cullProgram) {
        shader_program_destroy(ps->cullProgram);
        ps->cullProgram = NULL;
    }
    if (ps->densityClearProgram) {
        shader_program_destroy(ps->densityClearProgram);
        ps->densityClearProgram = NULL;
    }
    if (ps->densityAccumulateProgram) {
        shader_program_destroy(ps->densityAccumulateProgram);
        ps->densityAccumulateProgram = NULL;
    }
    if (ps->renderProgram) {
        shader_program_destroy(ps->renderProgram);
        ps->renderProgram = NULL;
    }
    if (ps->densityTexture) {
        glDeleteTextures(1, &ps->densityTexture);
        ps->densityTexture = 0;
    }
    if (ps->screenVAO) {
        glDeleteVertexArrays(1, &ps->screenVAO);
        ps->screenVAO = 0;
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

void particle_system_set_pixels_per_world(ParticleSystem* ps, float pixelsPerWorld) {
    ps->pixelsPerWorld = pixelsPerWorld;
}

void particle_system_set_viewport_size(ParticleSystem* ps, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }
    if (ps->viewportWidth == width && ps->viewportHeight == height && ps->densityTexture != 0) {
        return;
    }
    ps->viewportWidth = width;
    ps->viewportHeight = height;
    recreate_density_texture(ps);
}

void particle_system_set_view_bounds(ParticleSystem* ps, float minX, float minY, float maxX, float maxY) {
    ps->viewMin[0] = minX;
    ps->viewMin[1] = minY;
    ps->viewMax[0] = maxX;
    ps->viewMax[1] = maxY;
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
