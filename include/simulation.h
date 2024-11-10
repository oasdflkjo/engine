#ifndef SIMULATION_H
#define SIMULATION_H

#include <cglm/cglm.h>
#include "camera.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Simulation {
    struct SimulationVTable* vtable;
    float deltaTime;
    void* impl;
} Simulation;

typedef struct SimulationVTable {
    void (*init)(Simulation* sim);
    void (*cleanup)(Simulation* sim);
    void (*update)(Simulation* sim);
    void (*render)(Simulation* sim, mat4 view, mat4 projection);
    void (*set_gravity_point)(Simulation* sim, float x, float y);
    int (*get_particle_count)(Simulation* sim);
    float (*get_time_scale)(Simulation* sim);
    void (*set_time_scale)(Simulation* sim, float scale);
    float (*get_attraction_strength)(Simulation* sim);
    void (*set_attraction_strength)(Simulation* sim, float strength);
} SimulationVTable;

// Helper functions
static inline void simulation_init(Simulation* sim) {
    sim->vtable->init(sim);
}

static inline void simulation_cleanup(Simulation* sim) {
    sim->vtable->cleanup(sim);
}

static inline void simulation_update(Simulation* sim) {
    sim->vtable->update(sim);
}

static inline void simulation_render(Simulation* sim, mat4 view, mat4 projection) {
    sim->vtable->render(sim, view, projection);
}

static inline void simulation_set_gravity_point(Simulation* sim, float x, float y) {
    sim->vtable->set_gravity_point(sim, x, y);
}

static inline int simulation_get_particle_count(Simulation* sim) {
    return sim->vtable->get_particle_count(sim);
}

static inline float simulation_get_time_scale(Simulation* sim) {
    return sim->vtable->get_time_scale(sim);
}

static inline void simulation_set_time_scale(Simulation* sim, float scale) {
    sim->vtable->set_time_scale(sim, scale);
}

static inline float simulation_get_attraction_strength(Simulation* sim) {
    return sim->vtable->get_attraction_strength(sim);
}

static inline void simulation_set_attraction_strength(Simulation* sim, float strength) {
    sim->vtable->set_attraction_strength(sim, strength);
}

#ifdef __cplusplus
}
#endif

#endif // SIMULATION_H 