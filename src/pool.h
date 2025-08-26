#ifndef POOL_H
#define POOL_H

#include <stdint.h>
#include "particle.h"
#include "error.h"

/* Performance monitoring */
typedef struct {
    uint64_t allocations;
    uint64_t deallocations;
    uint64_t allocation_failures;
    double avg_allocation_time;
    double avg_deallocation_time;
} PoolStats;

/* Particle pool structure for efficient memory management */
typedef struct {
    Particle *pool;           /* Array of pre-allocated particles */
    int *free_indices;        /* Stack of available particle indices */
    int free_count;           /* Number of free particles available */
    int total_capacity;       /* Total capacity of the pool */
    int active_count;         /* Number of currently active particles */
    PoolStats stats;          /* Performance statistics */
} ParticlePool;

/* Pool iteration for active particles */
typedef struct {
    ParticlePool *pool;
    int current_index;
    int active_count;
    int total_active;
    int *active_indices;
} PoolIterator;

/* Pool management functions */
ParticlePool *pool_create(int capacity);
void pool_destroy(ParticlePool *pool);

/* Error-aware pool management functions */
Error pool_create_with_error(int capacity, ParticlePool **pool_out);
Error pool_allocate_particle_with_error(ParticlePool *pool, Particle **particle_out);
Error pool_free_particle_with_error(ParticlePool *pool, Particle *particle);
Error pool_iterator_create_with_error(ParticlePool *pool, PoolIterator *iter_out);
Error pool_create_with_error(int capacity, ParticlePool **pool_out);
Error pool_allocate_particle_with_error(ParticlePool *pool, Particle **particle_out);
Error pool_free_particle_with_error(ParticlePool *pool, Particle *particle);
Error pool_iterator_create_with_error(ParticlePool *pool, PoolIterator *iter_out);

/* Particle allocation/deallocation */
Particle *pool_allocate_particle(ParticlePool *pool);
void pool_free_particle(ParticlePool *pool, Particle *particle);

/* Pool statistics and utilities */
int pool_get_free_count(const ParticlePool *pool);
int pool_get_active_count(const ParticlePool *pool);
int pool_get_capacity(const ParticlePool *pool);
float pool_get_utilization(const ParticlePool *pool);

PoolIterator pool_iterator_create(ParticlePool *pool);
Particle *pool_iterator_next(PoolIterator *iter);
int pool_iterator_has_next(const PoolIterator *iter);
void pool_iterator_reset(PoolIterator *iter);
void pool_iterator_destroy(PoolIterator *iter);

PoolStats pool_get_stats(const ParticlePool *pool);
void pool_reset_stats(ParticlePool *pool);

/* Debug function */
void pool_print_status(const ParticlePool *pool);

#endif /* POOL_H */
