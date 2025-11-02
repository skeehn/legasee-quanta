#include "physics.h"
#include <math.h>
#include <string.h>

/* Helper: Calculate distance squared between two points */
static inline float dist_sq(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

/* Helper: Normalize vector */
static inline void normalize(float *x, float *y) {
    float len = sqrtf(*x * *x + *y * *y);
    if (len > 0.0001f) {
        *x /= len;
        *y /= len;
    }
}

/* Resolve collision between two particles */
static void resolve_particle_collision(Particle *p1, Particle *p2,
                                      CollisionSettings *settings) {
    float dx = p2->x - p1->x;
    float dy = p2->y - p1->y;
    float dist_sq_val = dx * dx + dy * dy;

    float min_dist = settings->collision_radius * 2.0f;
    float min_dist_sq = min_dist * min_dist;

    /* Check if particles are colliding */
    if (dist_sq_val >= min_dist_sq || dist_sq_val < 0.0001f) {
        return;
    }

    float dist = sqrtf(dist_sq_val);

    /* Collision normal (from p1 to p2) */
    float nx = dx / dist;
    float ny = dy / dist;

    /* Relative velocity */
    float dvx = p2->vx - p1->vx;
    float dvy = p2->vy - p1->vy;

    /* Relative velocity in collision normal direction */
    float dvn = dvx * nx + dvy * ny;

    /* Don't resolve if particles are separating */
    if (dvn >= 0) {
        return;
    }

    /* Calculate impulse (assuming equal mass) */
    float impulse = -(1.0f + settings->restitution) * dvn / 2.0f;

    /* Apply impulse */
    p1->vx -= impulse * nx * settings->friction;
    p1->vy -= impulse * ny * settings->friction;
    p2->vx += impulse * nx * settings->friction;
    p2->vy += impulse * ny * settings->friction;

    /* Separate particles to prevent overlap */
    float overlap = min_dist - dist;
    float separation = overlap * 0.5f;

    p1->x -= nx * separation;
    p1->y -= ny * separation;
    p2->x += nx * separation;
    p2->y += ny * separation;
}

/* Detect and resolve collisions */
int physics_resolve_collisions(SpatialGrid *grid, Particle **particles,
                               int num_particles, CollisionSettings *settings) {
    if (!grid || !particles || !settings || !settings->enabled) {
        return 0;
    }

    int collision_count = 0;

    /* Buffer for neighbor queries */
    #define MAX_NEIGHBORS 256
    Particle *neighbors[MAX_NEIGHBORS];

    /* Check each particle against its neighbors */
    for (int i = 0; i < num_particles; i++) {
        Particle *p1 = particles[i];
        if (!p1) continue;

        /* Get neighbors in 3x3 grid cells */
        int num_neighbors = spatial_grid_get_neighbors(grid, p1->x, p1->y,
                                                       neighbors, MAX_NEIGHBORS);

        /* Check collisions with neighbors */
        for (int j = 0; j < num_neighbors; j++) {
            Particle *p2 = neighbors[j];

            /* Skip self */
            if (p2 == p1) continue;

            /* Skip if we already checked this pair (p2 index < p1 index) */
            /* This assumes particles array is stable */
            if (p2 < p1) continue;

            resolve_particle_collision(p1, p2, settings);
            collision_count++;
        }
    }

    return collision_count;

    #undef MAX_NEIGHBORS
}

/* Apply radial force field */
static void apply_radial_force(Particle *p, ForceField *field, float dt) {
    float dx = p->x - field->x;
    float dy = p->y - field->y;
    float dist_sq_val = dx * dx + dy * dy;

    /* Check radius */
    if (field->radius > 0) {
        float radius_sq = field->radius * field->radius;
        if (dist_sq_val > radius_sq) return;
    }

    if (dist_sq_val < 0.0001f) return;

    float dist = sqrtf(dist_sq_val);
    float nx = dx / dist;
    float ny = dy / dist;

    /* Force falls off with distance */
    float force = field->strength / (1.0f + dist * 0.1f);

    p->vx += nx * force * dt;
    p->vy += ny * force * dt;
}

/* Apply directional force field */
static void apply_directional_force(Particle *p, ForceField *field, float dt) {
    p->vx += field->direction_x * field->strength * dt;
    p->vy += field->direction_y * field->strength * dt;
}

/* Apply vortex force field */
static void apply_vortex_force(Particle *p, ForceField *field, float dt) {
    float dx = p->x - field->x;
    float dy = p->y - field->y;
    float dist_sq_val = dx * dx + dy * dy;

    /* Check radius */
    if (field->radius > 0) {
        float radius_sq = field->radius * field->radius;
        if (dist_sq_val > radius_sq) return;
    }

    if (dist_sq_val < 0.0001f) return;

    float dist = sqrtf(dist_sq_val);

    /* Tangential direction (perpendicular to radius) */
    float tx = -dy / dist;
    float ty = dx / dist;

    /* Force falls off with distance */
    float force = field->strength / (1.0f + dist * 0.05f);

    p->vx += tx * force * dt;
    p->vy += ty * force * dt;
}

/* Apply attractor force field */
static void apply_attractor_force(Particle *p, ForceField *field, float dt) {
    float dx = field->x - p->x;
    float dy = field->y - p->y;
    float dist_sq_val = dx * dx + dy * dy;

    /* Check radius */
    if (field->radius > 0) {
        float radius_sq = field->radius * field->radius;
        if (dist_sq_val > radius_sq) return;
    }

    if (dist_sq_val < 1.0f) return;  /* Avoid singularity */

    float dist = sqrtf(dist_sq_val);
    float nx = dx / dist;
    float ny = dy / dist;

    /* Gravitational force: F = G / r² */
    float force = field->strength / dist_sq_val;

    p->vx += nx * force * dt;
    p->vy += ny * force * dt;
}

/* Apply single force field to particle */
void physics_apply_force_field(Particle *particle, ForceField *field, float dt) {
    if (!particle || !field || !field->active) {
        return;
    }

    switch (field->type) {
        case FORCE_FIELD_RADIAL:
            apply_radial_force(particle, field, dt);
            break;

        case FORCE_FIELD_DIRECTIONAL:
            apply_directional_force(particle, field, dt);
            break;

        case FORCE_FIELD_VORTEX:
            apply_vortex_force(particle, field, dt);
            break;

        case FORCE_FIELD_ATTRACTOR:
            apply_attractor_force(particle, field, dt);
            break;
    }
}

/* Apply multiple force fields to all particles */
void physics_apply_force_fields(Particle **particles, int num_particles,
                                ForceField *fields, int num_fields, float dt) {
    if (!particles || !fields) return;

    for (int i = 0; i < num_particles; i++) {
        if (!particles[i]) continue;

        for (int j = 0; j < num_fields; j++) {
            physics_apply_force_field(particles[i], &fields[j], dt);
        }
    }
}

/* Create radial force field */
ForceField physics_create_radial_field(float x, float y, float strength, float radius) {
    return (ForceField){
        .type = FORCE_FIELD_RADIAL,
        .x = x,
        .y = y,
        .strength = strength,
        .radius = radius,
        .active = true
    };
}

/* Create directional force field */
ForceField physics_create_directional_field(float dir_x, float dir_y, float strength) {
    float nx = dir_x, ny = dir_y;
    normalize(&nx, &ny);

    return (ForceField){
        .type = FORCE_FIELD_DIRECTIONAL,
        .direction_x = nx,
        .direction_y = ny,
        .strength = strength,
        .active = true
    };
}

/* Create vortex force field */
ForceField physics_create_vortex_field(float x, float y, float strength, float radius) {
    return (ForceField){
        .type = FORCE_FIELD_VORTEX,
        .x = x,
        .y = y,
        .strength = strength,
        .radius = radius,
        .active = true
    };
}

/* Create attractor force field */
ForceField physics_create_attractor_field(float x, float y, float strength, float radius) {
    return (ForceField){
        .type = FORCE_FIELD_ATTRACTOR,
        .x = x,
        .y = y,
        .strength = strength,
        .radius = radius,
        .active = true
    };
}
