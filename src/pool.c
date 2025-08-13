#include "pool.h"
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
        iter.active_count = 0;
        
        /* Precompute active indices for O(1) iteration */
        iter.active_indices = malloc(pool->total_capacity * sizeof(int));
        if (iter.active_indices) {
            int active_idx = 0;
            /* Mark all indices as potentially active */
            char *is_free = calloc(pool->total_capacity, sizeof(char));
            if (is_free) {
                /* Mark free indices */
                for (int i = 0; i < pool->free_count; i++) {
                    is_free[pool->free_indices[i]] = 1;
                }
                
                /* Collect active indices */
                for (int i = 0; i < pool->total_capacity; i++) {
                    if (!is_free[i]) {
                        iter.active_indices[active_idx++] = i;
                    }
                }
                
                iter.total_active = active_idx;
                free(is_free);
            }
        }
    }
    return iter;
}

/* Get next active particle from iterator */
Particle *pool_iterator_next(PoolIterator *iter) {
    if (!iter || !iter->pool || !iter->active_indices) {
        return NULL;
    }
    
    /* Return particle at current active index */
    if (iter->current_index < iter->total_active) {
        int particle_index = iter->active_indices[iter->current_index];
        iter->current_index++;
        iter->active_count++;
        return &iter->pool->pool[particle_index];
    }
    
    return NULL; /* No more active particles */
}

/* Check if iterator has more particles */
int pool_iterator_has_next(const PoolIterator *iter) {
    if (!iter || !iter->pool || !iter->active_indices) {
        return 0;
    }
    
    return iter->current_index < iter->total_active;
}

/* Reset iterator to beginning */
void pool_iterator_reset(PoolIterator *iter) {
    if (iter) {
        iter->current_index = 0;
        iter->active_count = 0;
    }
}

/* Destroy iterator and free resources */
void pool_iterator_destroy(PoolIterator *iter) {
    if (iter) {
        if (iter->active_indices) {
            free(iter->active_indices);
            iter->active_indices = NULL;
        }
        iter->pool = NULL;
        iter->current_index = 0;
        iter->active_count = 0;
        iter->total_active = 0;
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
    printf("  Allocations: %llu\n", stats.allocations);
    printf("  Deallocations: %llu\n", stats.deallocations);
    printf("  Failures: %llu\n", stats.allocation_failures);
    printf("  Avg Allocation Time: %.2f μs\n", stats.avg_allocation_time);
    printf("  Avg Deallocation Time: %.2f μs\n", stats.avg_deallocation_time);
}
