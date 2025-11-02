#ifndef SIM_H
#define SIM_H

#include <stdint.h>
#include <stdbool.h>
#include "particle.h"
#include "pool.h"
#include "error.h"
#include "spatial_grid.h"
#include "physics.h"

/* Simulation structure */
typedef struct {
    ParticlePool *pool;       /* Object pool for particles */
    int count, capacity;
    float gravity, windx, windy;
    int width, height;
    uint32_t rng_state;
    Particle *simd_buffer;    /* Cached SIMD working buffer */
    int simd_buffer_capacity; /* Number of particles the buffer can hold */

    /* Enhanced physics (Week 2) */
    SpatialGrid *spatial_grid;    /* Spatial partitioning for collision detection */
    CollisionSettings collision_settings;
    ForceField *force_fields;     /* Array of force fields */
    int num_force_fields;
    int force_fields_capacity;
    bool use_spatial_grid;        /* Enable/disable spatial grid optimization */
} Simulation;

/* Core simulation functions */
Simulation *sim_create(int capacity, int width, int height);
void sim_destroy(Simulation *sim);
void sim_step(Simulation *sim, float dt);
void sim_step_scalar(Simulation *sim, float dt); /* Scalar fallback implementation */
void sim_spawn_burst(Simulation *sim, float x, float y, int count, float spread);

/* Utility functions */
void sim_clear(Simulation *sim);
int sim_get_particle_count(const Simulation *sim);
void sim_set_gravity(Simulation *sim, float value);
void sim_set_wind(Simulation *sim, float x, float y);
float sim_get_gravity(const Simulation *sim);
void sim_get_wind(const Simulation *sim, float *x, float *y);

/* Physics utilities */
float sim_get_particle_speed(const Particle *p);
uint32_t sim_speed_to_color(float speed);

/* Particle access functions */
const Particle *sim_get_particle(const Simulation *sim, int index);
void sim_add_particle(Simulation *sim, float x, float y, float vx, float vy);

/* Pool integration functions */
ParticlePool *sim_get_pool(const Simulation *sim);
void sim_print_pool_stats(const Simulation *sim);

/* Error-aware simulation functions */
Error sim_create_with_error(int capacity, int width, int height, Simulation **sim_out);
Error sim_add_particle_with_error(Simulation *sim, float x, float y, float vx, float vy);
Error sim_spawn_burst_with_error(Simulation *sim, float x, float y, int count, float spread);
Error sim_step_with_error(Simulation *sim, float dt);

/* Enhanced physics functions (Week 2) */
void sim_enable_collisions(Simulation *sim, bool enable);
void sim_set_collision_settings(Simulation *sim, CollisionSettings settings);
CollisionSettings sim_get_collision_settings(const Simulation *sim);

int sim_add_force_field(Simulation *sim, ForceField field);
void sim_remove_force_field(Simulation *sim, int index);
void sim_clear_force_fields(Simulation *sim);
ForceField* sim_get_force_field(Simulation *sim, int index);
int sim_get_force_field_count(const Simulation *sim);

void sim_enable_spatial_grid(Simulation *sim, bool enable);
GridStats sim_get_grid_stats(const Simulation *sim);

#endif /* SIM_H */ 
