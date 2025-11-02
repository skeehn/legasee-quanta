#include "sim.h"
#include "pool.h"
#include "simd.h"
#include "error.h"
#include "spatial_grid.h"
#include "physics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* Fast PRNG (xorshift) for particle spawning */
static uint32_t xorshift32(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

/* Convert PRNG output to float in range [0, 1) */
static float rand_float(uint32_t *state) {
    return (float)xorshift32(state) / (float)UINT32_MAX;
}

/* Convert PRNG output to float in range [min, max) */
static float rand_range(uint32_t *state, float min, float max) {
    return min + (max - min) * rand_float(state);
}

/* Ensure the simulation has a SIMD buffer with at least required_count slots */
static Particle *sim_acquire_simd_buffer(Simulation *sim, int required_count) {
    if (!sim || required_count <= 0) {
        return NULL;
    }

    if (sim->simd_buffer && sim->simd_buffer_capacity >= required_count) {
        return sim->simd_buffer;
    }

    size_t alignment = simd_get_preferred_alignment();
    Particle *new_buffer = (Particle *)simd_aligned_alloc((size_t)required_count * sizeof(Particle), alignment);
    if (!new_buffer) {
        return NULL;
    }

    if (sim->simd_buffer) {
        simd_aligned_free(sim->simd_buffer);
    }

    sim->simd_buffer = new_buffer;
    sim->simd_buffer_capacity = required_count;
    return sim->simd_buffer;
}

/* Create a new simulation with specified capacity and dimensions */
Simulation *sim_create(int capacity, int width, int height) {
    Simulation *sim = malloc(sizeof(Simulation));
    if (!sim) {
        return NULL;
    }
    
    /* Create particle pool */
    sim->pool = pool_create(capacity);
    if (!sim->pool) {
        free(sim);
        return NULL;
    }
    
    sim->capacity = capacity;
    sim->count = 0;
    sim->width = width;
    sim->height = height;
    sim->simd_buffer = NULL;
    sim->simd_buffer_capacity = 0;
    
    /* Initialize physics parameters */
    sim->gravity = 30.0f;  /* pixels per second squared */
    sim->windx = 0.0f;
    sim->windy = 0.0f;
    
    /* Initialize PRNG with current time */
    sim->rng_state = (uint32_t)time(NULL);
    if (sim->rng_state == 0) {
        sim->rng_state = 1;  /* xorshift32 requires non-zero seed */
    }

    /* Initialize enhanced physics (Week 2) */
    sim->spatial_grid = spatial_grid_create(width, height, 10.0f);  /* 10-pixel cells */
    sim->collision_settings = physics_default_collision_settings();
    sim->collision_settings.enabled = false;  /* Disabled by default */
    sim->force_fields = NULL;
    sim->num_force_fields = 0;
    sim->force_fields_capacity = 0;
    sim->use_spatial_grid = false;  /* Disabled by default for backward compatibility */

    return sim;
}

/* Destroy simulation and free all memory */
void sim_destroy(Simulation *sim) {
    if (sim) {
        if (sim->simd_buffer) {
            simd_aligned_free(sim->simd_buffer);
        }
        if (sim->spatial_grid) {
            spatial_grid_destroy(sim->spatial_grid);
        }
        if (sim->force_fields) {
            free(sim->force_fields);
        }
        pool_destroy(sim->pool);
        free(sim);
    }
}

/* Clear all particles from simulation */
void sim_clear(Simulation *sim) {
    if (sim && sim->pool) {
        /* Free all active particles back to pool */
        PoolIterator iter = pool_iterator_create(sim->pool);
        Particle *particle;
        while ((particle = pool_iterator_next(&iter)) != NULL) {
            pool_free_particle(sim->pool, particle);
        }
        sim->count = 0;
        pool_iterator_destroy(&iter);
    }
}

/* Get current particle count */
int sim_get_particle_count(const Simulation *sim) {
    return sim && sim->pool ? pool_get_active_count(sim->pool) : 0;
}

/* Set gravity value */
void sim_set_gravity(Simulation *sim, float value) {
    if (sim) {
        sim->gravity = value;
    }
}

/* Set wind vector */
void sim_set_wind(Simulation *sim, float x, float y) {
    if (sim) {
        sim->windx = x;
        sim->windy = y;
    }
}

/* Get gravity value */
float sim_get_gravity(const Simulation *sim) {
    return sim ? sim->gravity : 0.0f;
}

/* Get wind vector */
void sim_get_wind(const Simulation *sim, float *x, float *y) {
    if (sim && x && y) {
        *x = sim->windx;
        *y = sim->windy;
    } else if (x && y) {
        *x = 0.0f;
        *y = 0.0f;
    }
}

/* Get particle speed */
float sim_get_particle_speed(const Particle *p) {
    return sqrtf(p->vx * p->vx + p->vy * p->vy);
}

/* Convert speed to color (brightness based on velocity) */
uint32_t sim_speed_to_color(float speed) {
    /* Clamp speed to reasonable range */
    speed = fminf(speed, 50.0f);
    float t = speed / 50.0f;
    
    /* Smoothstep for nicer color transition */
    t = t * t * (3.0f - 2.0f * t);
    
    /* Color ramp: dark blue -> cyan -> white */
    uint8_t r, g, b;
    if (t < 0.5f) {
        /* Dark blue to cyan */
        float local_t = t * 2.0f;
        r = (uint8_t)(0.0f * (1.0f - local_t) + 0.0f * local_t);
        g = (uint8_t)(64.0f * (1.0f - local_t) + 200.0f * local_t);
        b = (uint8_t)(160.0f * (1.0f - local_t) + 255.0f * local_t);
    } else {
        /* Cyan to white */
        float local_t = (t - 0.5f) * 2.0f;
        r = (uint8_t)(0.0f * (1.0f - local_t) + 255.0f * local_t);
        g = (uint8_t)(200.0f * (1.0f - local_t) + 255.0f * local_t);
        b = (uint8_t)(255.0f * (1.0f - local_t) + 255.0f * local_t);
    }
    
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* Spawn a burst of particles at specified location */
void sim_spawn_burst(Simulation *sim, float x, float y, int count, float spread) {
    if (!sim || !sim->pool) {
        return;
    }
    
    int spawn_count = count;
    int available = pool_get_free_count(sim->pool);
    if (spawn_count > available) {
        spawn_count = available;
    }
    
    for (int i = 0; i < spawn_count; i++) {
        Particle *p = pool_allocate_particle(sim->pool);
        if (!p) {
            break; /* Pool is full */
        }
        
        /* Set position */
        p->x = x;
        p->y = y;
        
        /* Random direction within spread angle */
        float angle = rand_range(&sim->rng_state, -spread, spread);
        float speed = rand_range(&sim->rng_state, 5.0f, 20.0f);
        
        /* Set velocity */
        p->vx = speed * cosf(angle);
        p->vy = speed * sinf(angle);
        
        sim->count++;
    }
}

/* Step simulation forward by dt seconds using Euler integration */
void sim_step(Simulation *sim, float dt) {
    if (!sim || !sim->pool) return;

    const float damping = 0.6f;  /* Velocity damping on wall collisions */
    const float friction = 0.98f; /* Ground friction */

    /* Get the best available SIMD function */
    simd_step_func_t simd_func = simd_select_step_function();

    int active_count = pool_get_active_count(sim->pool);
    if (active_count == 0) {
        return;
    }

    Particle *simd_buffer = sim_acquire_simd_buffer(sim, active_count);
    if (!simd_buffer) {
        /* Fallback to scalar processing if allocation fails */
        sim_step_scalar(sim, dt);
        return;
    }

    /* Copy particles to aligned buffer */
    PoolIterator iter = pool_iterator_create(sim->pool);
    Particle *p;
    int i = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        simd_buffer[i++] = *p;
    }

    /* Apply SIMD physics calculations */
    simd_func(simd_buffer, active_count, dt, sim->gravity, sim->windx, sim->windy);

    /* Apply force fields if any */
    if (sim->num_force_fields > 0 && sim->force_fields) {
        /* Create particle pointer array for force field application */
        Particle **particle_ptrs = (Particle**)malloc(sizeof(Particle*) * active_count);
        if (particle_ptrs) {
            for (int idx = 0; idx < active_count; idx++) {
                particle_ptrs[idx] = &simd_buffer[idx];
            }
            physics_apply_force_fields(particle_ptrs, active_count,
                                      sim->force_fields, sim->num_force_fields, dt);
            free(particle_ptrs);
        }
    }

    /* Copy back and handle collisions/cleanup */
    pool_iterator_reset(&iter);
    i = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        Particle *updated = &simd_buffer[i++];

        /* Copy updated physics data */
        p->x = updated->x;
        p->y = updated->y;
        p->vx = updated->vx;
        p->vy = updated->vy;

        /* Wall collision detection and response */
        if (p->x < 0) {
            p->x = 0;
            p->vx = -p->vx * damping;
        } else if (p->x >= sim->width - 1) {
            p->x = sim->width - 1;
            p->vx = -p->vx * damping;
        }

        if (p->y < 0) {
            p->y = 0;
            p->vy = -p->vy * damping;
        } else if (p->y >= sim->height - 1) {
            p->y = sim->height - 1;
            p->vy = -p->vy * damping;

            /* Apply ground friction when near bottom */
            if (fabsf(p->vy) < 2.0f) {
                p->vx *= friction;
            }
        }

        /* Remove particles that are too slow and near bottom */
        if (p->y >= sim->height - 2 && fabsf(p->vx) < 0.5f && fabsf(p->vy) < 0.5f) {
            pool_free_particle(sim->pool, p);
            sim->count--;
        }
    }

    pool_iterator_destroy(&iter);

    /* Handle particle-particle collisions if enabled */
    if (sim->use_spatial_grid && sim->collision_settings.enabled && sim->spatial_grid) {
        /* Build spatial grid */
        spatial_grid_clear(sim->spatial_grid);

        /* Create particle pointer array for spatial grid */
        Particle **particle_ptrs = (Particle**)malloc(sizeof(Particle*) * active_count);
        if (particle_ptrs) {
            iter = pool_iterator_create(sim->pool);
            i = 0;
            while ((p = pool_iterator_next(&iter)) != NULL) {
                particle_ptrs[i] = p;
                spatial_grid_insert(sim->spatial_grid, p);
                i++;
            }
            pool_iterator_destroy(&iter);

            /* Resolve collisions using spatial grid */
            physics_resolve_collisions(sim->spatial_grid, particle_ptrs,
                                      active_count, &sim->collision_settings);

            free(particle_ptrs);
        }
    }

    /* Synchronize cached count with pool */
    sim->count = pool_get_active_count(sim->pool);
}

/* Scalar fallback implementation */
void sim_step_scalar(Simulation *sim, float dt) {
    if (!sim || !sim->pool) return;
    
    const float damping = 0.6f;  /* Velocity damping on wall collisions */
    const float friction = 0.98f; /* Ground friction */
    
    /* Use iterator to process only active particles */
    PoolIterator iter = pool_iterator_create(sim->pool);
    Particle *p;
    
    while ((p = pool_iterator_next(&iter)) != NULL) {
        /* Apply forces */
        p->vx += sim->windx * dt;
        p->vy += (sim->gravity + sim->windy) * dt;
        
        /* Update position */
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        /* Wall collision detection and response */
        if (p->x < 0) {
            p->x = 0;
            p->vx = -p->vx * damping;
        } else if (p->x >= sim->width - 1) {
            p->x = sim->width - 1;
            p->vx = -p->vx * damping;
        }
        
        if (p->y < 0) {
            p->y = 0;
            p->vy = -p->vy * damping;
        } else if (p->y >= sim->height - 1) {
            p->y = sim->height - 1;
            p->vy = -p->vy * damping;
            
            /* Apply ground friction when near bottom */
            if (fabsf(p->vy) < 2.0f) {
                p->vx *= friction;
            }
        }
        
        /* Remove particles that are too slow and near bottom */
        if (p->y >= sim->height - 2 && fabsf(p->vx) < 0.5f && fabsf(p->vy) < 0.5f) {
            pool_free_particle(sim->pool, p);
            sim->count--;
        }
    }
    
    /* Clean up iterator */
    pool_iterator_destroy(&iter);

    /* Synchronize cached count with pool */
    sim->count = pool_get_active_count(sim->pool);
}

/* Get particle at index (for rendering) */
const Particle *sim_get_particle(const Simulation *sim, int index) {
    if (!sim || !sim->pool || index < 0) {
        return NULL;
    }
    
    /* Use iterator to find particle at specific index */
    PoolIterator iter = pool_iterator_create(sim->pool);
    Particle *p;
    int current_index = 0;
    
    while ((p = pool_iterator_next(&iter)) != NULL) {
        if (current_index == index) {
            pool_iterator_destroy(&iter);
            return p;
        }
        current_index++;
    }
    
    /* Clean up iterator */
    pool_iterator_destroy(&iter);
    
    return NULL; /* Index out of bounds */
}

/* Add a single particle at specified position and velocity */
void sim_add_particle(Simulation *sim, float x, float y, float vx, float vy) {
    if (!sim || !sim->pool) {
        return;
    }
    
    Particle *p = pool_allocate_particle(sim->pool);
    if (!p) {
        return; /* Pool is full */
    }
    
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    sim->count++;
}

/* Get the particle pool */
ParticlePool *sim_get_pool(const Simulation *sim) {
    return sim ? sim->pool : NULL;
}

/* Print pool statistics */
void sim_print_pool_stats(const Simulation *sim) {
    if (sim && sim->pool) {
        pool_print_status(sim->pool);
    }
}

/* ===== ERROR-AWARE SIMULATION FUNCTIONS ===== */

/* Create a new simulation with error handling */
Error sim_create_with_error(int capacity, int width, int height, Simulation **sim_out) {
    ERROR_CHECK(sim_out != NULL, ERROR_NULL_POINTER, "Simulation output pointer cannot be NULL");
    ERROR_CHECK(capacity > 0, ERROR_INVALID_PARAMETER, "Capacity must be positive");
    ERROR_CHECK(width > 0, ERROR_INVALID_PARAMETER, "Width must be positive");
    ERROR_CHECK(height > 0, ERROR_INVALID_PARAMETER, "Height must be positive");
    
    Simulation *sim = error_malloc(sizeof(Simulation));
    if (!sim) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate simulation structure");
    }
    
    /* Create particle pool with error handling */
    Error err = pool_create_with_error(capacity, &sim->pool);
    if (err.code != SUCCESS) {
        error_free(sim);
        return err;
    }
    
    sim->capacity = capacity;
    sim->count = 0;
    sim->width = width;
    sim->height = height;
    sim->simd_buffer = NULL;
    sim->simd_buffer_capacity = 0;

    /* Initialize physics parameters */
    sim->gravity = 30.0f;  /* pixels per second squared */
    sim->windx = 0.0f;
    sim->windy = 0.0f;

    /* Initialize PRNG with current time */
    sim->rng_state = (uint32_t)time(NULL);
    if (sim->rng_state == 0) {
        sim->rng_state = 1;  /* xorshift32 requires non-zero seed */
    }

    /* Initialize enhanced physics (Week 2) */
    sim->spatial_grid = spatial_grid_create(width, height, 10.0f);  /* 10-pixel cells */
    sim->collision_settings = physics_default_collision_settings();
    sim->collision_settings.enabled = false;  /* Disabled by default */
    sim->force_fields = NULL;
    sim->num_force_fields = 0;
    sim->force_fields_capacity = 0;
    sim->use_spatial_grid = false;  /* Disabled by default for backward compatibility */

    *sim_out = sim;
    return (Error){SUCCESS};
}

/* Add a single particle with error handling */
Error sim_add_particle_with_error(Simulation *sim, float x, float y, float vx, float vy) {
    ERROR_CHECK(sim != NULL, ERROR_NULL_POINTER, "Simulation cannot be NULL");
    ERROR_CHECK(sim->pool != NULL, ERROR_NULL_POINTER, "Particle pool cannot be NULL");
    ERROR_CHECK(x >= 0.0f && x < sim->width, ERROR_OUT_OF_RANGE, "X position out of bounds");
    ERROR_CHECK(y >= 0.0f && y < sim->height, ERROR_OUT_OF_RANGE, "Y position out of bounds");
    
    Particle *particle = NULL;
    Error err = pool_allocate_particle_with_error(sim->pool, &particle);
    if (err.code != SUCCESS) {
        return err;
    }
    
    particle->x = x;
    particle->y = y;
    particle->vx = vx;
    particle->vy = vy;
    sim->count++;
    
    return (Error){SUCCESS};
}

/* Spawn a burst of particles with error handling */
Error sim_spawn_burst_with_error(Simulation *sim, float x, float y, int count, float spread) {
    ERROR_CHECK(sim != NULL, ERROR_NULL_POINTER, "Simulation cannot be NULL");
    ERROR_CHECK(sim->pool != NULL, ERROR_NULL_POINTER, "Particle pool cannot be NULL");
    ERROR_CHECK(count > 0, ERROR_INVALID_PARAMETER, "Burst count must be positive");
    ERROR_CHECK(x >= 0.0f && x < sim->width, ERROR_OUT_OF_RANGE, "X position out of bounds");
    ERROR_CHECK(y >= 0.0f && y < sim->height, ERROR_OUT_OF_RANGE, "Y position out of bounds");
    ERROR_CHECK(spread >= 0.0f && spread <= 2.0f * M_PI, ERROR_OUT_OF_RANGE, "Spread angle out of range");
    
    int spawn_count = count;
    int available = pool_get_free_count(sim->pool);
    if (spawn_count > available) {
        spawn_count = available;
    }
    
    int spawned = 0;
    for (int i = 0; i < spawn_count; i++) {
        Particle *particle = NULL;
        Error err = pool_allocate_particle_with_error(sim->pool, &particle);
        if (err.code != SUCCESS) {
            break; /* Pool is full */
        }
        
        /* Set position */
        particle->x = x;
        particle->y = y;
        
        /* Random direction within spread angle */
        float angle = rand_range(&sim->rng_state, -spread, spread);
        float speed = rand_range(&sim->rng_state, 5.0f, 20.0f);
        
        /* Set velocity */
        particle->vx = speed * cosf(angle);
        particle->vy = speed * sinf(angle);
        
        sim->count++;
        spawned++;
    }
    
    if (spawned == 0) {
        return ERROR_CREATE(ERROR_OUT_OF_RESOURCES, "No particles could be spawned");
    }
    
    return (Error){SUCCESS};
}

/* Step simulation with error handling */
Error sim_step_with_error(Simulation *sim, float dt) {
    ERROR_CHECK(sim != NULL, ERROR_NULL_POINTER, "Simulation cannot be NULL");
    ERROR_CHECK(sim->pool != NULL, ERROR_NULL_POINTER, "Particle pool cannot be NULL");
    ERROR_CHECK(dt > 0.0f, ERROR_INVALID_PARAMETER, "Time step must be positive");

    const float damping = 0.6f;  /* Velocity damping on wall collisions */
    const float friction = 0.98f; /* Ground friction */

    /* Get the best available SIMD function with error handling */
    simd_step_func_t simd_func;
    Error err = simd_select_step_function_with_error(&simd_func);
    if (err.code != SUCCESS) {
        return err;
    }

    int active_count = pool_get_active_count(sim->pool);
    if (active_count == 0) {
        return (Error){SUCCESS};
    }

    Particle *simd_buffer = sim_acquire_simd_buffer(sim, active_count);
    if (!simd_buffer) {
        /* Fallback gracefully to scalar path if allocation fails */
        sim_step_scalar(sim, dt);
        return (Error){SUCCESS};
    }

    PoolIterator iter = pool_iterator_create(sim->pool);
    Particle *p;
    int idx = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        simd_buffer[idx++] = *p;
    }

    simd_func(simd_buffer, active_count, dt, sim->gravity, sim->windx, sim->windy);

    pool_iterator_reset(&iter);
    idx = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        Particle *updated = &simd_buffer[idx++];
        p->x = updated->x;
        p->y = updated->y;
        p->vx = updated->vx;
        p->vy = updated->vy;

        if (p->x < 0) {
            p->x = 0;
            p->vx = -p->vx * damping;
        } else if (p->x >= sim->width - 1) {
            p->x = sim->width - 1;
            p->vx = -p->vx * damping;
        }

        if (p->y < 0) {
            p->y = 0;
            p->vy = -p->vy * damping;
        } else if (p->y >= sim->height - 1) {
            p->y = sim->height - 1;
            p->vy = -p->vy * damping;

            if (fabsf(p->vy) < 2.0f) {
                p->vx *= friction;
            }
        }

        if (p->y >= sim->height - 2 && fabsf(p->vx) < 0.5f && fabsf(p->vy) < 0.5f) {
            pool_free_particle_with_error(sim->pool, p);
            sim->count--;
        }
    }

    pool_iterator_destroy(&iter);
    sim->count = pool_get_active_count(sim->pool);

    return (Error){SUCCESS};
}

/* ===== ENHANCED PHYSICS FUNCTIONS (Week 2) ===== */

/* Enable/disable collisions */
void sim_enable_collisions(Simulation *sim, bool enable) {
    if (sim) {
        sim->collision_settings.enabled = enable;
        sim->use_spatial_grid = enable;  /* Auto-enable spatial grid for collisions */
    }
}

/* Set collision settings */
void sim_set_collision_settings(Simulation *sim, CollisionSettings settings) {
    if (sim) {
        sim->collision_settings = settings;
    }
}

/* Get collision settings */
CollisionSettings sim_get_collision_settings(const Simulation *sim) {
    if (sim) {
        return sim->collision_settings;
    }
    return physics_default_collision_settings();
}

/* Add force field */
int sim_add_force_field(Simulation *sim, ForceField field) {
    if (!sim) return -1;

    /* Expand capacity if needed */
    if (sim->num_force_fields >= sim->force_fields_capacity) {
        int new_capacity = (sim->force_fields_capacity == 0) ? 4 : sim->force_fields_capacity * 2;
        ForceField *new_fields = (ForceField*)realloc(sim->force_fields,
                                                       sizeof(ForceField) * new_capacity);
        if (!new_fields) return -1;

        sim->force_fields = new_fields;
        sim->force_fields_capacity = new_capacity;
    }

    /* Add field */
    sim->force_fields[sim->num_force_fields] = field;
    return sim->num_force_fields++;
}

/* Remove force field by index */
void sim_remove_force_field(Simulation *sim, int index) {
    if (!sim || index < 0 || index >= sim->num_force_fields) return;

    /* Shift remaining fields down */
    for (int i = index; i < sim->num_force_fields - 1; i++) {
        sim->force_fields[i] = sim->force_fields[i + 1];
    }

    sim->num_force_fields--;
}

/* Clear all force fields */
void sim_clear_force_fields(Simulation *sim) {
    if (sim) {
        sim->num_force_fields = 0;
    }
}

/* Get force field by index */
ForceField* sim_get_force_field(Simulation *sim, int index) {
    if (!sim || index < 0 || index >= sim->num_force_fields) return NULL;
    return &sim->force_fields[index];
}

/* Get force field count */
int sim_get_force_field_count(const Simulation *sim) {
    return sim ? sim->num_force_fields : 0;
}

/* Enable/disable spatial grid */
void sim_enable_spatial_grid(Simulation *sim, bool enable) {
    if (sim) {
        sim->use_spatial_grid = enable;
    }
}

/* Get grid statistics */
GridStats sim_get_grid_stats(const Simulation *sim) {
    GridStats stats = {0};
    if (sim && sim->spatial_grid) {
        spatial_grid_get_stats(sim->spatial_grid, &stats);
    }
    return stats;
}
