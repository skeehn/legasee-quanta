#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/error.h"
#include "../src/pool.h"
#include "../src/simd.h"
#include "../src/sim.h"
#include "../src/term.h"
#include "../src/render.h"
#include "../src/input.h"
#include "../src/particle.h"

int main(void) {
    printf("=== ASCII Particle Simulator - Error Handling Integration Test ===\n\n");
    
    /* Initialize error handling system */
    error_init();
    printf("Error system initialized!\n\n");
    
    int passed_tests = 0;
    int failed_tests = 0;
    
    /* Test 1: Pool Error Handling Integration */
    printf("Test 1: Pool Error Handling Integration\n");
    ParticlePool *pool = NULL;
    Error err = pool_create_with_error(1000, &pool);
    
    if (err.code == SUCCESS && pool != NULL) {
        printf("  ✓ Pool creation: PASSED\n");
        passed_tests++;
        
        /* Test particle allocation */
        Particle *particle = NULL;
        err = pool_allocate_particle_with_error(pool, &particle);
        if (err.code == SUCCESS && particle != NULL) {
            printf("  ✓ Particle allocation: PASSED\n");
            passed_tests++;
            
            /* Test particle deallocation */
            err = pool_free_particle_with_error(pool, particle);
            if (err.code == SUCCESS) {
                printf("  ✓ Particle deallocation: PASSED\n");
                passed_tests++;
            } else {
                printf("  ✗ Particle deallocation: FAILED\n");
                error_print(&err);
                failed_tests++;
            }
        } else {
            printf("  ✗ Particle allocation: FAILED\n");
            error_print(&err);
            failed_tests++;
        }
        
        pool_destroy(pool);
    } else {
        printf("  ✗ Pool creation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 2: SIMD Error Handling Integration */
    printf("Test 2: SIMD Error Handling Integration\n");
    SIMDCapabilities caps;
    err = simd_detect_capabilities_with_error(&caps);
    
    if (err.code == SUCCESS) {
        printf("  ✓ SIMD capability detection: PASSED\n");
        passed_tests++;
        
        /* Test aligned memory allocation */
        void *aligned_ptr = NULL;
        err = simd_aligned_alloc_with_error(1024, 16, &aligned_ptr);
        if (err.code == SUCCESS && aligned_ptr != NULL) {
            printf("  ✓ Aligned memory allocation: PASSED\n");
            passed_tests++;
            simd_aligned_free(aligned_ptr);
        } else {
            printf("  ✗ Aligned memory allocation: FAILED\n");
            error_print(&err);
            failed_tests++;
        }
    } else {
        printf("  ✗ SIMD capability detection: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 3: Simulation Error Handling Integration */
    printf("Test 3: Simulation Error Handling Integration\n");
    Simulation *sim = NULL;
    err = sim_create_with_error(500, 80, 24, &sim);
    
    if (err.code == SUCCESS && sim != NULL) {
        printf("  ✓ Simulation creation: PASSED\n");
        passed_tests++;
        
        /* Test particle addition */
        err = sim_add_particle_with_error(sim, 40.0f, 12.0f, 5.0f, -2.0f);
        if (err.code == SUCCESS) {
            printf("  ✓ Particle addition: PASSED\n");
            passed_tests++;
            
            /* Test particle burst */
            err = sim_spawn_burst_with_error(sim, 40.0f, 12.0f, 10, 0.5f);
            if (err.code == SUCCESS) {
                printf("  ✓ Particle burst: PASSED\n");
                passed_tests++;
                
                /* Test simulation step */
                err = sim_step_with_error(sim, 1.0f / 60.0f);
                if (err.code == SUCCESS) {
                    printf("  ✓ Simulation step: PASSED\n");
                    passed_tests++;
                } else {
                    printf("  ✗ Simulation step: FAILED\n");
                    error_print(&err);
                    failed_tests++;
                }
            } else {
                printf("  ✗ Particle burst: FAILED\n");
                error_print(&err);
                failed_tests++;
            }
        } else {
            printf("  ✗ Particle addition: FAILED\n");
            error_print(&err);
            failed_tests++;
        }
        
        sim_destroy(sim);
    } else {
        printf("  ✗ Simulation creation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 4: Terminal Error Handling Integration */
    printf("Test 4: Terminal Error Handling Integration\n");
    int width, height;
    err = term_get_size_with_error(&width, &height);

    if (err.code == SUCCESS) {
        printf("  ✓ Terminal size detection: PASSED (%dx%d)\n", width, height);
        passed_tests++;

        /* Test terminal initialization (skip if already initialized) */
        int initialized;
        err = term_is_initialized_with_error(&initialized);
        if (err.code == SUCCESS) {
            printf("  ✓ Terminal initialization check: PASSED\n");
            passed_tests++;
        } else {
            printf("  ✗ Terminal initialization check: FAILED\n");
            error_print(&err);
            failed_tests++;
        }
    } else if (err.code == ERROR_SYSTEM_ERROR) {
        /* Expected in non-interactive environments (CI, piped output, etc.) */
        printf("  ✓ Terminal size detection: PASSED (non-interactive, using defaults: %dx%d)\n", width, height);
        passed_tests++;
    } else {
        printf("  ✗ Terminal size detection: FAILED (unexpected error)\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 5: Renderer Error Handling Integration */
    printf("Test 5: Renderer Error Handling Integration\n");
    Renderer *renderer = NULL;
    err = renderer_create_with_error(80, 24, &renderer);
    
    if (err.code == SUCCESS && renderer != NULL) {
        printf("  ✓ Renderer creation: PASSED\n");
        passed_tests++;
        
        /* Test plotting */
        err = renderer_plot_with_error(renderer, 40, 12, '@', 0xFF0000);
        if (err.code == SUCCESS) {
            printf("  ✓ Renderer plotting: PASSED\n");
            passed_tests++;
            
            /* Test text drawing */
            err = renderer_draw_text_with_error(renderer, 0, 0, "Test", 0x00FF00);
            if (err.code == SUCCESS) {
                printf("  ✓ Renderer text drawing: PASSED\n");
                passed_tests++;
                
                /* Test size retrieval */
                int r_width, r_height;
                err = renderer_get_size_with_error(renderer, &r_width, &r_height);
                if (err.code == SUCCESS) {
                    printf("  ✓ Renderer size retrieval: PASSED (%dx%d)\n", r_width, r_height);
                    passed_tests++;
                } else {
                    printf("  ✗ Renderer size retrieval: FAILED\n");
                    error_print(&err);
                    failed_tests++;
                }
            } else {
                printf("  ✗ Renderer text drawing: FAILED\n");
                error_print(&err);
                failed_tests++;
            }
        } else {
            printf("  ✗ Renderer plotting: FAILED\n");
            error_print(&err);
            failed_tests++;
        }
        
        renderer_destroy(renderer);
    } else {
        printf("  ✗ Renderer creation: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 6: Input Error Handling Integration */
    printf("Test 6: Input Error Handling Integration\n");
    UIState ui;
    err = input_init_state_with_error(&ui);
    
    if (err.code == SUCCESS) {
        printf("  ✓ Input state initialization: PASSED\n");
        passed_tests++;
        
        /* Test status text generation */
        char status_buffer[256];
        Simulation *test_sim = sim_create(100, 80, 24);
        if (test_sim) {
            err = input_get_status_text_with_error(test_sim, &ui, status_buffer, sizeof(status_buffer));
            if (err.code == SUCCESS) {
                printf("  ✓ Status text generation: PASSED\n");
                passed_tests++;
            } else {
                printf("  ✗ Status text generation: FAILED\n");
                error_print(&err);
                failed_tests++;
            }
            sim_destroy(test_sim);
        } else {
            printf("  ✗ Test simulation creation failed\n");
            failed_tests++;
        }
    } else {
        printf("  ✗ Input state initialization: FAILED\n");
        error_print(&err);
        failed_tests++;
    }
    
    /* Test 7: Error Propagation and Integration */
    printf("Test 7: Error Propagation and Integration\n");
    
    /* Test invalid parameters */
    err = pool_create_with_error(-1, &pool);
    if (err.code == ERROR_INVALID_PARAMETER) {
        printf("  ✓ Invalid parameter error: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Invalid parameter error: FAILED\n");
        failed_tests++;
    }
    
    err = sim_create_with_error(100, -1, 24, &sim);
    if (err.code == ERROR_INVALID_PARAMETER) {
        printf("  ✓ Invalid simulation parameters: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Invalid simulation parameters: FAILED\n");
        failed_tests++;
    }
    
    err = renderer_create_with_error(0, 24, &renderer);
    if (err.code == ERROR_INVALID_PARAMETER) {
        printf("  ✓ Invalid renderer parameters: PASSED\n");
        passed_tests++;
    } else {
        printf("  ✗ Invalid renderer parameters: FAILED\n");
        failed_tests++;
    }
    
    /* Cleanup */
    error_cleanup();
    
    /* Print results */
    printf("\n=== Integration Test Results ===\n");
    printf("Total Tests: %d\n", passed_tests + failed_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Success Rate: %.1f%%\n", 
           (passed_tests + failed_tests) > 0 ? 
           (float)passed_tests / (passed_tests + failed_tests) * 100.0f : 0.0f);
    
    /* Print final error statistics */
    printf("\n=== Final Error Statistics ===\n");
    ErrorStats stats = error_get_stats();
    printf("Total Errors: %zu\n", stats.total_errors);
    printf("Memory Errors: %zu\n", stats.memory_errors);
    printf("Parameter Errors: %zu\n", stats.parameter_errors);
    printf("=======================\n");
    
    return (failed_tests == 0) ? 0 : 1;
}
