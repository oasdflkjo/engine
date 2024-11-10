#ifndef PARTICLE_SIMULATION_H
#define PARTICLE_SIMULATION_H

#include "simulation.h"
#include "particle_system.h"

#ifdef __cplusplus
extern "C" {
#endif

// Create a simulation implementation for the particle system
typedef struct ParticleSimulation {
    Simulation base;           // Must be first member
    ParticleSystem particles;  // Actual implementation
} ParticleSimulation;

// Create/destroy functions
ParticleSimulation* particle_simulation_create(void);
void particle_simulation_destroy(ParticleSimulation* sim);

#ifdef __cplusplus
}
#endif

#endif // PARTICLE_SIMULATION_H 