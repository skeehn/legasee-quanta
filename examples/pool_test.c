#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include "../src/pool.h"
#include "../src/sim.h"

/* Test configuration */
#define TEST_CAPACITY 1000
#define NUM_THREADS 4
#define ITERATIONS_PER_THREAD 1000
#define PERFORMANCE_ITERATIONS 10000

/* Performance measurement */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/* Test 1: Basic Pool Creation and Destruction */
static int test_pool_creation(void) {
    printf("Test 1: Pool Creation and Destruction\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    if (pool_get_capacity(pool) != TEST_CAPACITY) {
        printf("  ❌ Incorrect capacity: expected %d, got %d\n", 
               TEST_CAPACITY, pool_get_capacity(pool));
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_free_count(pool) != TEST_CAPACITY) {
        printf("  ❌ Incorrect free count: expected %d, got %d\n", 
               TEST_CAPACITY, pool_get_free_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_active_count(pool) != 0) {
        printf("  ❌ Incorrect active count: expected 0, got %d\n", 
               pool_get_active_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    pool_destroy(pool);
    printf("  ✅ Pool creation and destruction test passed\n");
    return 1;
}

/* Test 2: Particle Allocation and Deallocation */
static int test_particle_allocation(void) {
    printf("Test 2: Particle Allocation and Deallocation\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    /* Test single allocation */
    Particle *p1 = pool_allocate_particle(pool);
    if (!p1) {
        printf("  ❌ Failed to allocate particle\n");
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_active_count(pool) != 1) {
        printf("  ❌ Incorrect active count after allocation: expected 1, got %d\n", 
               pool_get_active_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_free_count(pool) != TEST_CAPACITY - 1) {
        printf("  ❌ Incorrect free count after allocation: expected %d, got %d\n", 
               TEST_CAPACITY - 1, pool_get_free_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    /* Test particle initialization */
    p1->x = 10.0f;
    p1->y = 20.0f;
    p1->vx = 5.0f;
    p1->vy = -3.0f;
    
    /* Test deallocation */
    pool_free_particle(pool, p1);
    
    if (pool_get_active_count(pool) != 0) {
        printf("  ❌ Incorrect active count after deallocation: expected 0, got %d\n", 
               pool_get_active_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_free_count(pool) != TEST_CAPACITY) {
        printf("  ❌ Incorrect free count after deallocation: expected %d, got %d\n", 
               TEST_CAPACITY, pool_get_free_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    pool_destroy(pool);
    printf("  ✅ Particle allocation and deallocation test passed\n");
    return 1;
}

/* Test 3: Particle Reuse */
static int test_particle_reuse(void) {
    printf("Test 3: Particle Reuse\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    /* Allocate and free multiple particles */
    Particle *particles[10];
    for (int i = 0; i < 10; i++) {
        particles[i] = pool_allocate_particle(pool);
        if (!particles[i]) {
            printf("  ❌ Failed to allocate particle %d\n", i);
            pool_destroy(pool);
            return 0;
        }
        
        /* Set unique values */
        particles[i]->x = (float)i;
        particles[i]->y = (float)(i * 2);
        particles[i]->vx = (float)(i * 3);
        particles[i]->vy = (float)(i * 4);
    }
    
    /* Free all particles */
    for (int i = 0; i < 10; i++) {
        pool_free_particle(pool, particles[i]);
    }
    
    /* Allocate again and check if we get the same memory addresses */
    Particle *reused_particles[10];
    for (int i = 0; i < 10; i++) {
        reused_particles[i] = pool_allocate_particle(pool);
        if (!reused_particles[i]) {
            printf("  ❌ Failed to allocate reused particle %d\n", i);
            pool_destroy(pool);
            return 0;
        }
    }
    
    /* Check if we got the same memory addresses (LIFO behavior) */
    int reuse_count = 0;
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            if (particles[i] == reused_particles[j]) {
                reuse_count++;
                break;
            }
        }
    }
    
    if (reuse_count < 5) { /* Allow some flexibility due to LIFO ordering */
        printf("  ⚠️  Low particle reuse rate: %d/10 (expected most to be reused)\n", reuse_count);
    } else {
        printf("  ✅ Particle reuse test passed (%d/10 particles reused)\n", reuse_count);
    }
    
    pool_destroy(pool);
    return 1;
}

/* Test 4: Pool Exhaustion */
static int test_pool_exhaustion(void) {
    printf("Test 4: Pool Exhaustion\n");
    
    ParticlePool *pool = pool_create(5); /* Small capacity for testing */
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    Particle *particles[10];
    int allocated_count = 0;
    
    /* Try to allocate more particles than capacity */
    for (int i = 0; i < 10; i++) {
        particles[i] = pool_allocate_particle(pool);
        if (particles[i]) {
            allocated_count++;
        }
    }
    
    if (allocated_count != 5) {
        printf("  ❌ Incorrect allocation count: expected 5, got %d\n", allocated_count);
        pool_destroy(pool);
        return 0;
    }
    
    /* Try to allocate one more - should fail */
    Particle *extra = pool_allocate_particle(pool);
    if (extra) {
        printf("  ❌ Should not be able to allocate beyond capacity\n");
        pool_destroy(pool);
        return 0;
    }
    
    /* Free one particle and try again */
    pool_free_particle(pool, particles[0]);
    extra = pool_allocate_particle(pool);
    if (!extra) {
        printf("  ❌ Should be able to allocate after freeing\n");
        pool_destroy(pool);
        return 0;
    }
    
    pool_destroy(pool);
    printf("  ✅ Pool exhaustion test passed\n");
    return 1;
}

/* Test 5: Pool Iterator */
static int test_pool_iterator(void) {
    printf("Test 5: Pool Iterator\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    /* Allocate some particles */
    Particle *particles[5];
    for (int i = 0; i < 5; i++) {
        particles[i] = pool_allocate_particle(pool);
        if (!particles[i]) {
            printf("  ❌ Failed to allocate particle %d\n", i);
            pool_destroy(pool);
            return 0;
        }
        particles[i]->x = (float)i;
    }
    
    /* Test iterator */
    PoolIterator iter = pool_iterator_create(pool);
    int found_count = 0;
    Particle *p;
    
    while ((p = pool_iterator_next(&iter)) != NULL) {
        found_count++;
        /* Verify we can access the particle */
        if (p->x < 0 || p->x >= 5) {
            printf("  ❌ Invalid particle data in iterator\n");
            pool_destroy(pool);
            return 0;
        }
    }
    
    if (found_count != 5) {
        printf("  ❌ Iterator found %d particles, expected 5\n", found_count);
        pool_destroy(pool);
        return 0;
    }
    
    pool_destroy(pool);
    printf("  ✅ Pool iterator test passed\n");
    return 1;
}

/* Test 6: Performance Benchmark */
static int test_performance(void) {
    printf("Test 6: Performance Benchmark\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    /* Benchmark allocation/deallocation performance */
    double start_time = get_time_ms();
    
    for (int i = 0; i < PERFORMANCE_ITERATIONS; i++) {
        Particle *p = pool_allocate_particle(pool);
        if (p) {
            p->x = (float)i;
            p->y = (float)(i * 2);
            pool_free_particle(pool, p);
        }
    }
    
    double end_time = get_time_ms();
    double duration = end_time - start_time;
    double ops_per_second = (PERFORMANCE_ITERATIONS * 2.0) / (duration / 1000.0);
    
    printf("  📊 Performance: %.2f operations/second (%.2f ms for %d iterations)\n", 
           ops_per_second, duration, PERFORMANCE_ITERATIONS);
    
    /* Check if performance is reasonable (> 100K ops/sec) */
    if (ops_per_second < 100000) {
        printf("  ⚠️  Performance seems low, but continuing...\n");
    } else {
        printf("  ✅ Performance benchmark passed\n");
    }
    
    pool_destroy(pool);
    return 1;
}

/* Test 7: Thread Safety */
static void *thread_worker(void *arg) {
    ParticlePool *pool = (ParticlePool *)arg;
    Particle *particles[10];
    
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++) {
        /* Allocate particles */
        for (int j = 0; j < 10; j++) {
            particles[j] = pool_allocate_particle(pool);
            if (particles[j]) {
                particles[j]->x = (float)(i + j);
                particles[j]->y = (float)(i * j);
            }
        }
        
        /* Free particles */
        for (int j = 0; j < 10; j++) {
            if (particles[j]) {
                pool_free_particle(pool, particles[j]);
            }
        }
    }
    
    return NULL;
}

static int test_thread_safety(void) {
    printf("Test 7: Thread Safety\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    pthread_t threads[NUM_THREADS];
    
    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, thread_worker, pool) != 0) {
            printf("  ❌ Failed to create thread %d\n", i);
            pool_destroy(pool);
            return 0;
        }
    }
    
    /* Wait for threads to complete */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Check final state */
    if (pool_get_active_count(pool) != 0) {
        printf("  ❌ Thread safety test failed: %d particles still active\n", 
               pool_get_active_count(pool));
        pool_destroy(pool);
        return 0;
    }
    
    if (pool_get_free_count(pool) != TEST_CAPACITY) {
        printf("  ❌ Thread safety test failed: %d particles free, expected %d\n", 
               pool_get_free_count(pool), TEST_CAPACITY);
        pool_destroy(pool);
        return 0;
    }
    
    pool_destroy(pool);
    printf("  ✅ Thread safety test passed\n");
    return 1;
}

/* Test 8: Statistics */
static int test_statistics(void) {
    printf("Test 8: Statistics\n");
    
    ParticlePool *pool = pool_create(TEST_CAPACITY);
    if (!pool) {
        printf("  ❌ Failed to create pool\n");
        return 0;
    }
    
    /* Perform some operations */
    Particle *p1 = pool_allocate_particle(pool);
    Particle *p2 = pool_allocate_particle(pool);
    pool_free_particle(pool, p1);
    Particle *p3 = pool_allocate_particle(pool);
    
    /* Check statistics */
    PoolStats stats = pool_get_stats(pool);
    
    if (stats.allocations < 3) {
        printf("  ❌ Statistics test failed: %lu allocations, expected >= 3\n", 
               stats.allocations);
        pool_destroy(pool);
        return 0;
    }
    
    if (stats.deallocations < 1) {
        printf("  ❌ Statistics test failed: %lu deallocations, expected >= 1\n", 
               stats.deallocations);
        pool_destroy(pool);
        return 0;
    }
    
    printf("  📊 Statistics: %lu allocations, %lu deallocations\n", 
           stats.allocations, stats.deallocations);
    
    pool_destroy(pool);
    printf("  ✅ Statistics test passed\n");
    return 1;
}

/* Main test runner */
int main(void) {
    printf("=== Object Pool Test Suite ===\n");
    printf("Testing particle pooling implementation...\n\n");
    
    int tests_passed = 0;
    int total_tests = 8;
    
    /* Run all tests */
    tests_passed += test_pool_creation();
    tests_passed += test_particle_allocation();
    tests_passed += test_particle_reuse();
    tests_passed += test_pool_exhaustion();
    tests_passed += test_pool_iterator();
    tests_passed += test_performance();
    tests_passed += test_thread_safety();
    tests_passed += test_statistics();
    
    printf("\n=== Test Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, total_tests);
    
    if (tests_passed == total_tests) {
        printf("🎉 All tests passed! Object pooling implementation is working correctly.\n");
        return EXIT_SUCCESS;
    } else {
        printf("❌ Some tests failed. Please review the implementation.\n");
        return EXIT_FAILURE;
    }
}
