#ifndef PHYSICS_H
#define PHYSICS_H

#include "particle.h"
#include "spatial_grid.h"
#include "error.h"
#include <stdbool.h>

/**
 * Advanced Physics System
 *
 * Collision Detection:
 * - Particle-particle collisions using spatial grid
 * - Elastic collision response with momentum conservation
 * - Configurable collision radius and restitution
 *
 * Force Fields:
 * - Radial: Push/pull from center point
 * - Directional: Constant force in one direction (wind, gravity)
 * - Vortex: Rotational force around center point
 * - Attractor: Gravitational-like point attraction
 */

/* Force field types */
typedef enum {
    FORCE_FIELD_RADIAL,      /* Radial push/pull from center */
    FORCE_FIELD_DIRECTIONAL, /* Constant directional force */
    FORCE_FIELD_VORTEX,      /* Rotational force */
    FORCE_FIELD_ATTRACTOR    /* Point attraction (gravity-like) */
} ForceFieldType;

/* Force field structure */
typedef struct {
    ForceFieldType type;
    float x, y;              /* Center position */
    float strength;          /* Force strength */
    float radius;            /* Effective radius (0 = infinite) */
    float direction_x;       /* Direction vector (for directional) */
    float direction_y;
    bool active;             /* Whether field is active */
} ForceField;

/* Collision settings */
typedef struct {
    float collision_radius;  /* Particle collision radius */
    float restitution;       /* Elasticity (0 = inelastic, 1 = elastic) */
    float friction;          /* Velocity damping on collision (0-1) */
    bool enabled;            /* Whether collisions are enabled */
} CollisionSettings;

/**
 * Detect and resolve collisions between particles
 *
 * Uses spatial grid for O(n) performance instead of O(n²)
 *
 * @param grid Spatial grid containing particles
 * @param particles Array of all particles
 * @param num_particles Number of particles
 * @param settings Collision settings
 * @return Number of collisions resolved
 */
int physics_resolve_collisions(SpatialGrid *grid, Particle **particles,
                               int num_particles, CollisionSettings *settings);

/**
 * Apply force field to particle
 *
 * @param particle Particle to apply force to
 * @param field Force field
 * @param dt Time delta
 */
void physics_apply_force_field(Particle *particle, ForceField *field, float dt);

/**
 * Apply multiple force fields to all particles
 *
 * @param particles Array of particles
 * @param num_particles Number of particles
 * @param fields Array of force fields
 * @param num_fields Number of force fields
 * @param dt Time delta
 */
void physics_apply_force_fields(Particle **particles, int num_particles,
                                ForceField *fields, int num_fields, float dt);

/**
 * Create radial force field (push/pull from center)
 *
 * @param x Center X
 * @param y Center Y
 * @param strength Force strength (positive = repel, negative = attract)
 * @param radius Effective radius (0 = infinite)
 */
ForceField physics_create_radial_field(float x, float y, float strength, float radius);

/**
 * Create directional force field (wind, gravity)
 *
 * @param dir_x Direction X component
 * @param dir_y Direction Y component
 * @param strength Force strength
 */
ForceField physics_create_directional_field(float dir_x, float dir_y, float strength);

/**
 * Create vortex force field (rotation around center)
 *
 * @param x Center X
 * @param y Center Y
 * @param strength Angular velocity
 * @param radius Effective radius (0 = infinite)
 */
ForceField physics_create_vortex_field(float x, float y, float strength, float radius);

/**
 * Create attractor force field (gravitational)
 *
 * @param x Center X
 * @param y Center Y
 * @param strength Gravitational constant
 * @param radius Effective radius (0 = infinite)
 */
ForceField physics_create_attractor_field(float x, float y, float strength, float radius);

/**
 * Default collision settings
 */
static inline CollisionSettings physics_default_collision_settings(void) {
    return (CollisionSettings){
        .collision_radius = 1.0f,
        .restitution = 0.8f,
        .friction = 0.95f,
        .enabled = true
    };
}

#endif /* PHYSICS_H */
