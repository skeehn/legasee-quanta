#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/error.h"
#include "../src/pool.h"
#include "../src/particle.h"

int main() {
    printf("=== Pool Error Handling Integration Test ===\n\n");
    
    /* Initialize error handling system */
    error_init();
    printf("Error system initialized!\n\n");
    
    int passed_tests = 0;
    int failed_tests = 0;
    
    /* Test 1: Pool creation with error handling */
    printf("Test 1: Pool Creation with Error Handling\n");
    ParticlePool *pool = NULL;
    Error err = pool_create_with_error(1000, &pool);
    
    if (err.code == SUCCESS && pool != NULL) {
        printf("  ✓ Pool creation: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Pool creation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 2: Particle allocation with error handling */
    printf("Test 2: Particle Allocation with Error Handling\n");
    Particle *particle = NULL;
    err = pool_allocate_particle_with_error(pool, &particle);
    
    if (err.code == SUCCESS && particle != NULL) {
        printf("  ✓ Particle allocation: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Particle allocation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 3: Particle deallocation with error handling */
    printf("Test 3: Particle Deallocation with Error Handling\n");
    err = pool_free_particle_with_error(pool, particle);
    
    if (err.code == SUCCESS) {
        printf("  ✓ Particle deallocation: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Particle deallocation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 4: Iterator creation with error handling */
    printf("Test 4: Iterator Creation with Error Handling\n");
    PoolIterator iter;
    err = pool_iterator_create_with_error(pool, &iter);
    
    if (err.code == SUCCESS) {
        printf("  ✓ Iterator creation: PASSED\n");
        passed_tests++;
        pool_iterator_destroy(&iter);
    } else {
        printf("  ✗ Iterator creation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 5: Error handling for invalid parameters */
    printf("Test 5: Invalid Parameter Error Handling\n");
    
    /* Test invalid capacity */
    ParticlePool *invalid_pool = NULL;
    err = pool_create_with_error(-1, &invalid_pool);
    if (err.code == ERROR_INVALID_PARAMETER) {
        printf("  ✓ Invalid capacity error: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Invalid capacity error: FAILED\n");
        failed_tests++;
    }
    
    /* Test NULL output pointer */
    err = pool_create_with_error(100, NULL);
    if (err.code == ERROR_NULL_POINTER) {
        printf("  ✓ NULL pointer error: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ NULL pointer error: FAILED\n");
        failed_tests++;
    }
    
    /* Test 6: Out of resources error */
    printf("Test 6: Out of Resources Error Handling\n");
    
    /* Create a small pool and exhaust it */
    ParticlePool *small_pool = NULL;
    err = pool_create_with_error(1, &small_pool);
    if (err.code == SUCCESS) {
        Particle *p1 = NULL, *p2 = NULL;
        
        /* First allocation should succeed */
        err = pool_allocate_particle_with_error(small_pool, &p1);
        if (err.code == SUCCESS) {
            /* Second allocation should fail */
            err = pool_allocate_particle_with_error(small_pool, &p2);
            if (err.code == ERROR_OUT_OF_RANGE) {
                printf("  ✓ Out of resources error: PASSED\n");
                passed_tests++;
            } else {
                printf("  ✗ Out of resources error: FAILED\n");
                failed_tests++;
            }
            
            pool_free_particle_with_error(small_pool, p1);
        }
        pool_destroy(small_pool);
    }
    
    /* Test 7: Invalid particle pointer error */
    printf("Test 7: Invalid Particle Pointer Error Handling\n");
    Particle invalid_particle = {0};
    err = pool_free_particle_with_error(pool, &invalid_particle);
    if (err.code == ERROR_INVALID_PARAMETER) {
        printf("  ✓ Invalid particle pointer error: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Invalid particle pointer error: FAILED\n");
        failed_tests++;
    }
    
    /* Cleanup */
    pool_destroy(pool);
    error_cleanup();
    
    /* Print results */
    printf("\n=== Test Results ===\n");
    printf("Total Tests: %d\n", passed_tests + failed_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Success Rate: %.1f%%\n", 
           (passed_tests + failed_tests) > 0 ? 
           (float)passed_tests / (passed_tests + failed_tests) * 100.0f : 0.0f);
    
    /* Print error statistics */
    printf("\n=== Error Statistics ===\n");
    ErrorStats stats = error_get_stats();
    printf("Total Errors: %zu\n", stats.total_errors);
    printf("Memory Errors: %zu\n", stats.memory_errors);
    printf("Parameter Errors: %zu\n", stats.parameter_errors);
    printf("=======================\n");
    
    return (failed_tests == 0) ? 0 : 1;
}
