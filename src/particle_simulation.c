#include "particle_simulation.h"
#include <stdlib.h>

// VTable implementation
static void particle_sim_init(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    particle_system_init(&psim->particles);
}

static void particle_sim_cleanup(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    particle_system_cleanup(&psim->particles);
}

static void particle_sim_update(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    psim->particles.deltaTime = sim->deltaTime;
    particle_system_update(&psim->particles);
}

static void particle_sim_render(Simulation* sim, mat4 view, mat4 projection) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    particle_system_render(&psim->particles, view, projection);
}

static void particle_sim_set_gravity_point(Simulation* sim, float x, float y) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    particle_system_set_gravity_point(&psim->particles, x, y);
}

static int particle_sim_get_particle_count(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    return psim->particles.count;
}

static float particle_sim_get_time_scale(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    return psim->particles.timeScale;
}

static void particle_sim_set_time_scale(Simulation* sim, float scale) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    psim->particles.timeScale = scale;
}

static float particle_sim_get_attraction_strength(Simulation* sim) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    return psim->particles.attractionStrength;
}

static void particle_sim_set_attraction_strength(Simulation* sim, float strength) {
    ParticleSimulation* psim = (ParticleSimulation*)sim;
    psim->particles.attractionStrength = strength;
}

// Static VTable
static SimulationVTable particle_sim_vtable = {
    .init = particle_sim_init,
    .cleanup = particle_sim_cleanup,
    .update = particle_sim_update,
    .render = particle_sim_render,
    .set_gravity_point = particle_sim_set_gravity_point,
    .get_particle_count = particle_sim_get_particle_count,
    .get_time_scale = particle_sim_get_time_scale,
    .set_time_scale = particle_sim_set_time_scale,
    .get_attraction_strength = particle_sim_get_attraction_strength,
    .set_attraction_strength = particle_sim_set_attraction_strength
};

// Creation/destruction functions
ParticleSimulation* particle_simulation_create(void) {
    ParticleSimulation* sim = malloc(sizeof(ParticleSimulation));
    if (!sim) return NULL;
    
    sim->base.vtable = &particle_sim_vtable;
    sim->base.deltaTime = 0.0f;
    sim->base.impl = sim;
    
    return sim;
}

void particle_simulation_destroy(ParticleSimulation* sim) {
    if (sim) {
        simulation_cleanup((Simulation*)sim);
        free(sim);
    }
} 