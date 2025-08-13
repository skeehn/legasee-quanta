#include "sim.h"
#include "pool.h"
#include "simd.h"
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
    
    /* Initialize physics parameters */
    sim->gravity = 30.0f;  /* pixels per second squared */
    sim->windx = 0.0f;
    sim->windy = 0.0f;
    
    /* Initialize PRNG with current time */
    sim->rng_state = (uint32_t)time(NULL);
    
    return sim;
}

/* Destroy simulation and free all memory */
void sim_destroy(Simulation *sim) {
    if (sim) {
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
    
    /* Collect all active particles into a contiguous array for SIMD processing */
    PoolIterator iter = pool_iterator_create(sim->pool);
    Particle *p;
    int active_count = 0;
    
    /* First pass: count active particles */
    while ((p = pool_iterator_next(&iter)) != NULL) {
        active_count++;
    }
    
    /* Clean up iterator */
    pool_iterator_destroy(&iter);
    
    if (active_count == 0) return;
    
    /* Allocate aligned buffer for SIMD processing */
    const size_t alignment = simd_get_preferred_alignment();
    Particle *simd_buffer = (Particle *)simd_aligned_alloc(active_count * sizeof(Particle), alignment);
    if (!simd_buffer) {
        /* Fallback to scalar processing if allocation fails */
        sim_step_scalar(sim, dt);
        return;
    }
    
    /* Second pass: copy particles to aligned buffer */
    iter = pool_iterator_create(sim->pool);
    int i = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        simd_buffer[i] = *p;
        i++;
    }
    
    /* Clean up iterator */
    pool_iterator_destroy(&iter);
    
    /* Apply SIMD physics calculations */
    simd_func(simd_buffer, active_count, dt, sim->gravity, sim->windx, sim->windy);
    
    /* Third pass: copy back and handle collisions/cleanup */
    iter = pool_iterator_create(sim->pool);
    i = 0;
    while ((p = pool_iterator_next(&iter)) != NULL) {
        /* Copy updated physics data */
        p->x = simd_buffer[i].x;
        p->y = simd_buffer[i].y;
        p->vx = simd_buffer[i].vx;
        p->vy = simd_buffer[i].vy;
        
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
        
        i++;
    }
    
    /* Clean up iterator */
    pool_iterator_destroy(&iter);
    
    /* Free SIMD buffer */
    simd_aligned_free(simd_buffer);
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
