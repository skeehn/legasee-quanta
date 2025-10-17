#include "pool.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* Performance monitoring */
static double get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000.0 + tv.tv_usec;
}

/* Create a new particle pool with specified capacity */
ParticlePool *pool_create(int capacity) {
    if (capacity <= 0) {
        return NULL;
    }
    
    ParticlePool *pool = malloc(sizeof(ParticlePool));
    if (!pool) {
        return NULL;
    }
    
    /* Allocate particle array */
    pool->pool = malloc(capacity * sizeof(Particle));
    if (!pool->pool) {
        free(pool);
        return NULL;
    }

    /* Allocate free indices stack */
    pool->free_indices = malloc(capacity * sizeof(int));
    if (!pool->free_indices) {
        free(pool->pool);
        free(pool);
        return NULL;
    }

    /* Allocate occupancy flags for fast iteration */
    pool->active_flags = calloc(capacity, sizeof(uint8_t));
    if (!pool->active_flags) {
        free(pool->free_indices);
        free(pool->pool);
        free(pool);
        return NULL;
    }
    
    /* Initialize pool state */
    pool->total_capacity = capacity;
    pool->active_count = 0;
    pool->free_count = capacity;
    
    /* Initialize free indices stack (LIFO for better cache locality) */
    for (int i = 0; i < capacity; i++) {
        pool->free_indices[i] = capacity - 1 - i; /* Reverse order for LIFO */
    }
    
    /* Initialize statistics */
    pool->stats.allocations = 0;
    pool->stats.deallocations = 0;
    pool->stats.allocation_failures = 0;
    pool->stats.avg_allocation_time = 0.0;
    pool->stats.avg_deallocation_time = 0.0;
    
    return pool;
}

/* Destroy particle pool and free all memory */
void pool_destroy(ParticlePool *pool) {
    if (pool) {
        free(pool->pool);
        free(pool->free_indices);
        free(pool->active_flags);
        free(pool);
    }
}

/* Allocate a particle from the pool */
Particle *pool_allocate_particle(ParticlePool *pool) {
    if (!pool || pool->free_count == 0) {
        if (pool) {
            pool->stats.allocation_failures++;
        }
        return NULL;
    }
    
    double start_time = get_time_us();
    
    /* Pop from free indices stack */
    pool->free_count--;
    int index = pool->free_indices[pool->free_count];
    
    /* Get particle and initialize it */
    Particle *particle = &pool->pool[index];
    memset(particle, 0, sizeof(Particle));
    pool->active_flags[index] = 1;

    /* Update statistics */
    pool->active_count++;
    pool->stats.allocations++;
    
    double end_time = get_time_us();
    double duration = end_time - start_time;
    
    /* Update average allocation time */
    if (pool->stats.allocations == 1) {
        pool->stats.avg_allocation_time = duration;
    } else {
        pool->stats.avg_allocation_time = 
            (pool->stats.avg_allocation_time * (pool->stats.allocations - 1) + duration) / pool->stats.allocations;
    }
    
    return particle;
}

/* Free a particle back to the pool */
void pool_free_particle(ParticlePool *pool, Particle *particle) {
    if (!pool || !particle) {
        return;
    }
    
    /* Calculate particle index */
    int index = particle - pool->pool;
    if (index < 0 || index >= pool->total_capacity) {
        return; /* Invalid particle pointer */
    }
    
    double start_time = get_time_us();
    
    /* Push to free indices stack */
    pool->free_indices[pool->free_count] = index;
    pool->free_count++;
    pool->active_flags[index] = 0;

    /* Update statistics */
    pool->active_count--;
    pool->stats.deallocations++;
    
    double end_time = get_time_us();
    double duration = end_time - start_time;
    
    /* Update average deallocation time */
    if (pool->stats.deallocations == 1) {
        pool->stats.avg_deallocation_time = duration;
    } else {
        pool->stats.avg_deallocation_time = 
            (pool->stats.avg_deallocation_time * (pool->stats.deallocations - 1) + duration) / pool->stats.deallocations;
    }
}

/* Get number of free particles */
int pool_get_free_count(const ParticlePool *pool) {
    return pool ? pool->free_count : 0;
}

/* Get number of active particles */
int pool_get_active_count(const ParticlePool *pool) {
    return pool ? pool->active_count : 0;
}

/* Get total pool capacity */
int pool_get_capacity(const ParticlePool *pool) {
    return pool ? pool->total_capacity : 0;
}

/* Get pool utilization percentage */
float pool_get_utilization(const ParticlePool *pool) {
    if (!pool || pool->total_capacity == 0) {
        return 0.0f;
    }
    return (float)pool->active_count / (float)pool->total_capacity * 100.0f;
}

/* Create iterator for active particles */
PoolIterator pool_iterator_create(ParticlePool *pool) {
    PoolIterator iter = {0};
    if (pool) {
        iter.pool = pool;
        iter.current_index = 0;
    }
    return iter;
}

/* Get next active particle from iterator */
Particle *pool_iterator_next(PoolIterator *iter) {
    if (!iter || !iter->pool) {
        return NULL;
    }

    ParticlePool *pool = iter->pool;
    while (iter->current_index < pool->total_capacity) {
        int particle_index = iter->current_index++;
        if (pool->active_flags[particle_index]) {
            return &pool->pool[particle_index];
        }
    }

    return NULL; /* No more active particles */
}

/* Check if iterator has more particles */
int pool_iterator_has_next(const PoolIterator *iter) {
    if (!iter || !iter->pool) {
        return 0;
    }

    ParticlePool *pool = iter->pool;
    int index = iter->current_index;
    while (index < pool->total_capacity) {
        if (pool->active_flags[index]) {
            return 1;
        }
        index++;
    }
    return 0;
}

/* Reset iterator to beginning */
void pool_iterator_reset(PoolIterator *iter) {
    if (iter) {
        iter->current_index = 0;
    }
}

/* Destroy iterator and free resources */
void pool_iterator_destroy(PoolIterator *iter) {
    if (iter) {
        iter->pool = NULL;
        iter->current_index = 0;
    }
}

/* Get pool statistics */
PoolStats pool_get_stats(const ParticlePool *pool) {
    PoolStats stats = {0};
    if (pool) {
        stats = pool->stats;
    }
    return stats;
}

/* Reset pool statistics */
void pool_reset_stats(ParticlePool *pool) {
    if (pool) {
        pool->stats.allocations = 0;
        pool->stats.deallocations = 0;
        pool->stats.allocation_failures = 0;
        pool->stats.avg_allocation_time = 0.0;
        pool->stats.avg_deallocation_time = 0.0;
    }
}

/* Debug function to print pool status */
void pool_print_status(const ParticlePool *pool) {
    if (!pool) {
        printf("Pool: NULL\n");
        return;
    }
    
    PoolStats stats = pool_get_stats(pool);
    printf("Pool Status:\n");
    printf("  Capacity: %d\n", pool->total_capacity);
    printf("  Active: %d\n", pool->active_count);
    printf("  Free: %d\n", pool->free_count);
    printf("  Utilization: %.1f%%\n", pool_get_utilization(pool));
    printf("  Allocations: %lu\n", (unsigned long)stats.allocations);
    printf("  Deallocations: %lu\n", (unsigned long)stats.deallocations);
    printf("  Failures: %lu\n", (unsigned long)stats.allocation_failures);
    printf("  Avg Allocation Time: %.2f μs\n", stats.avg_allocation_time);
    printf("  Avg Deallocation Time: %.2f μs\n", stats.avg_deallocation_time);
}

/* ===== ERROR-AWARE POOL FUNCTIONS ===== */

/* Create a new particle pool with error handling */
Error pool_create_with_error(int capacity, ParticlePool **pool_out) {
    ERROR_CHECK_CONDITION(capacity > 0, ERROR_INVALID_PARAMETER, "Pool capacity must be positive");
    ERROR_CHECK_NULL(pool_out, "Pool output pointer");
    
    ParticlePool *pool = error_malloc(sizeof(ParticlePool));
    if (pool == NULL) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate pool structure");
    }
    
    /* Allocate particle array */
    pool->pool = error_malloc(capacity * sizeof(Particle));
    if (!pool->pool) {
        error_free(pool);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate particle array");
    }

    /* Allocate free indices stack */
    pool->free_indices = error_malloc(capacity * sizeof(int));
    if (!pool->free_indices) {
        error_free(pool->pool);
        error_free(pool);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate free indices array");
    }

    /* Allocate occupancy flags */
    pool->active_flags = error_calloc(capacity, sizeof(uint8_t));
    if (!pool->active_flags) {
        error_free(pool->free_indices);
        error_free(pool->pool);
        error_free(pool);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate active flags");
    }
    
    /* Initialize pool state */
    pool->total_capacity = capacity;
    pool->active_count = 0;
    pool->free_count = capacity;
    
    /* Initialize free indices stack (LIFO for better cache locality) */
    for (int i = 0; i < capacity; i++) {
        pool->free_indices[i] = capacity - 1 - i; /* Reverse order for LIFO */
    }
    
    /* Initialize statistics */
    pool->stats.allocations = 0;
    pool->stats.deallocations = 0;
    pool->stats.allocation_failures = 0;
    pool->stats.avg_allocation_time = 0.0;
    pool->stats.avg_deallocation_time = 0.0;
    
    *pool_out = pool;
    return (Error){SUCCESS, NULL, NULL, 0, NULL};
}

/* Allocate a particle from the pool with error handling */
Error pool_allocate_particle_with_error(ParticlePool *pool, Particle **particle_out) {
    ERROR_CHECK_NULL(pool, "Pool");
    ERROR_CHECK_NULL(particle_out, "Particle output pointer");
    ERROR_CHECK_CONDITION(pool->free_count > 0, ERROR_OUT_OF_RANGE, "No free particles available in pool");

    double start_time = get_time_us();

    /* Pop from free indices stack */
    pool->free_count--;
    int index = pool->free_indices[pool->free_count];

    /* Get particle and initialize it */
    Particle *particle = &pool->pool[index];
    memset(particle, 0, sizeof(Particle));
    pool->active_flags[index] = 1;

    /* Update statistics */
    pool->active_count++;
    pool->stats.allocations++;

    double end_time = get_time_us();
    double duration = end_time - start_time;

    /* Update average allocation time */
    if (pool->stats.allocations == 1) {
        pool->stats.avg_allocation_time = duration;
    } else {
        pool->stats.avg_allocation_time =
            (pool->stats.avg_allocation_time * (pool->stats.allocations - 1) + duration) / pool->stats.allocations;
    }

    *particle_out = particle;
    return (Error){SUCCESS, NULL, NULL, 0, NULL};
}

/* Free a particle back to the pool with error handling */
Error pool_free_particle_with_error(ParticlePool *pool, Particle *particle) {
    ERROR_CHECK_NULL(pool, "Pool");
    ERROR_CHECK_NULL(particle, "Particle");

    /* Calculate particle index */
    int index = particle - pool->pool;
    ERROR_CHECK_CONDITION(index >= 0 && index < pool->total_capacity, ERROR_INVALID_PARAMETER, "Invalid particle pointer");

    double start_time = get_time_us();

    /* Push to free indices stack */
    pool->free_indices[pool->free_count] = index;
    pool->free_count++;
    pool->active_flags[index] = 0;

    /* Update statistics */
    pool->active_count--;
    pool->stats.deallocations++;

    double end_time = get_time_us();
    double duration = end_time - start_time;

    /* Update average deallocation time */
    if (pool->stats.deallocations == 1) {
        pool->stats.avg_deallocation_time = duration;
    } else {
        pool->stats.avg_deallocation_time =
            (pool->stats.avg_deallocation_time * (pool->stats.deallocations - 1) + duration) / pool->stats.deallocations;
    }

    return (Error){SUCCESS, NULL, NULL, 0, NULL};
}

/* Create iterator for active particles with error handling */
Error pool_iterator_create_with_error(ParticlePool *pool, PoolIterator *iter_out) {
    ERROR_CHECK_NULL(pool, "Pool");
    ERROR_CHECK_NULL(iter_out, "Iterator output pointer");

    PoolIterator iter = {0};
    iter.pool = pool;
    iter.current_index = 0;

    *iter_out = iter;
    return (Error){SUCCESS, NULL, NULL, 0, NULL};
}
