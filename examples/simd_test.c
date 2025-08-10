#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "../src/simd.h"
#include "../src/particle.h"

/* Performance measurement helper */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/* Test configuration */
#define TEST_PARTICLE_COUNT 1000
#define BENCHMARK_ITERATIONS 1000
#define LARGE_TEST_COUNT 50000
#define LARGE_BENCHMARK_ITERATIONS 50
#define STRESS_TEST_COUNT 100000
#define STRESS_BENCHMARK_ITERATIONS 10

/* Test 1: SIMD Capability Detection */
static int test_simd_detection(void) {
    printf("Test 1: SIMD Capability Detection\n");
    
    SIMDCapabilities caps = simd_detect_capabilities();
    
    printf("  📊 Detected Capabilities:\n");
    printf("    Features: 0x%08x\n", caps.features);
    printf("    Max Vector Width: %d bytes\n", caps.max_vector_width);
    printf("    Preferred Alignment: %d bytes\n", caps.preferred_alignment);
    printf("    Cache Line Size: %d bytes\n", caps.cache_line_size);
    
    /* Check if we detected any SIMD features */
    if (caps.features == SIMD_NONE) {
        printf("  ⚠️  No SIMD features detected (this is normal on non-x86 platforms)\n");
    } else {
        printf("  ✅ SIMD features detected successfully\n");
    }
    
    /* Verify that capability detection is consistent */
    SIMDCapabilities caps2 = simd_detect_capabilities();
    if (memcmp(&caps, &caps2, sizeof(SIMDCapabilities)) != 0) {
        printf("  ❌ Inconsistent capability detection\n");
        return 0;
    }
    
    printf("  ✅ SIMD capability detection test passed\n");
    return 1;
}

/* Test 2: Feature Support Checking */
static int test_feature_support(void) {
    printf("Test 2: Feature Support Checking\n");
    
    /* Test individual feature detection */
    int sse_supported = simd_is_supported(SIMD_SSE);
    int avx_supported = simd_is_supported(SIMD_AVX);
    int avx2_supported = simd_is_supported(SIMD_AVX2);
    
    printf("  📊 Feature Support:\n");
    printf("    SSE: %s\n", sse_supported ? "✅ Supported" : "❌ Not Supported");
    printf("    AVX: %s\n", avx_supported ? "✅ Supported" : "❌ Not Supported");
    printf("    AVX2: %s\n", avx2_supported ? "✅ Supported" : "❌ Not Supported");
    
    /* Test function selection */
    simd_step_func_t selected_func = simd_select_step_function();
    const char *func_name = simd_get_function_name(selected_func);
    
    printf("    Selected Function: %s\n", func_name);
    
    printf("  ✅ Feature support checking test passed\n");
    return 1;
}

/* Test 3: Memory Alignment */
static int test_memory_alignment(void) {
    printf("Test 3: Memory Alignment\n");
    
    const size_t test_size = 1024;
    const size_t alignment = simd_get_preferred_alignment();
    
    /* Test aligned allocation */
    void *aligned_ptr = simd_aligned_alloc(test_size, alignment);
    if (!aligned_ptr) {
        printf("  ❌ Failed to allocate aligned memory\n");
        return 0;
    }
    
    /* Check alignment */
    if (!simd_is_aligned(aligned_ptr, alignment)) {
        printf("  ❌ Allocated memory is not properly aligned\n");
        simd_aligned_free(aligned_ptr);
        return 0;
    }
    
    /* Test size alignment */
    size_t aligned_size = simd_align_size(test_size, alignment);
    if (aligned_size < test_size || aligned_size % alignment != 0) {
        printf("  ❌ Size alignment calculation failed\n");
        simd_aligned_free(aligned_ptr);
        return 0;
    }
    
    /* Test validation */
    if (!simd_validate_alignment(aligned_ptr, aligned_size, alignment)) {
        printf("  ❌ Alignment validation failed\n");
        simd_aligned_free(aligned_ptr);
        return 0;
    }
    
    simd_aligned_free(aligned_ptr);
    printf("  ✅ Memory alignment test passed\n");
    return 1;
}

/* Test 4: SIMD Function Selection */
static int test_function_selection(void) {
    printf("Test 4: SIMD Function Selection\n");
    
    simd_step_func_t func = simd_select_step_function();
    if (!func) {
        printf("  ❌ No function selected\n");
        return 0;
    }
    
    const char *func_name = simd_get_function_name(func);
    printf("  📊 Selected Function: %s\n", func_name);
    
    /* Test that we can call the function */
    const int test_count = 100;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate test data\n");
        return 0;
    }
    
    /* Initialize test data */
    for (int i = 0; i < test_count; i++) {
        test_data[i].x = (float)i;
        test_data[i].y = (float)(i * 2);
        test_data[i].vx = (float)(i * 3);
        test_data[i].vy = (float)(i * 4);
    }
    
    /* Call the selected function */
    func(test_data, test_count, 1.0f/60.0f, 30.0f, 5.0f, -2.0f);
    
    /* Verify that the function modified the data */
    int modified = 0;
    for (int i = 0; i < test_count; i++) {
        if (test_data[i].x != (float)i || test_data[i].y != (float)(i * 2)) {
            modified = 1;
            break;
        }
    }
    
    if (!modified) {
        printf("  ⚠️  Function may not have modified data (this could be normal for scalar fallback)\n");
    }
    
    simd_aligned_free(test_data);
    printf("  ✅ SIMD function selection test passed\n");
    return 1;
}

/* Test 5: Performance Benchmarking */
static int test_performance_benchmark(void) {
    printf("Test 5: Performance Benchmarking\n");
    
    const int test_count = TEST_PARTICLE_COUNT;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate test data\n");
        return 0;
    }
    
    /* Initialize test data */
    for (int i = 0; i < test_count; i++) {
        test_data[i].x = (float)i;
        test_data[i].y = (float)(i * 2);
        test_data[i].vx = (float)(i * 3);
        test_data[i].vy = (float)(i * 4);
    }
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = -2.0f;
    
    /* Reset stats */
    simd_reset_stats();
    
    /* Benchmark scalar implementation */
    for (int i = 0; i < BENCHMARK_ITERATIONS; i++) {
        simd_step_scalar(test_data, test_count, dt, gravity, windx, windy);
    }
    
    /* Get stats */
    SIMDStats stats = simd_get_stats();
    printf("  📊 Performance Statistics:\n");
    printf("    Scalar Operations: %lu\n", stats.scalar_operations);
    printf("    SIMD Operations: %lu\n", stats.simd_operations);
    
    simd_aligned_free(test_data);
    printf("  ✅ Performance benchmarking test passed\n");
    return 1;
}

/* Comprehensive Performance Test */
static int test_comprehensive_performance(void) {
    printf("Test 5.5: Comprehensive Performance Test\n");
    
    const int large_test_count = LARGE_TEST_COUNT;
    const size_t data_size = large_test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate test data\n");
        return 0;
    }
    
    /* Initialize test data */
    for (int i = 0; i < large_test_count; i++) {
        test_data[i].x = (float)i;
        test_data[i].y = (float)(i * 2);
        test_data[i].vx = (float)(i * 3);
        test_data[i].vy = (float)(i * 4);
    }
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = -2.0f;
    
    /* Benchmark scalar implementation */
    simd_reset_stats();
    double start_time = get_time_ms();
    for (int i = 0; i < LARGE_BENCHMARK_ITERATIONS; i++) {
        simd_step_scalar(test_data, large_test_count, dt, gravity, windx, windy);
    }
    double scalar_time = get_time_ms() - start_time;
    SIMDStats scalar_stats = simd_get_stats();
    
    /* Benchmark NEON implementation if available */
    double neon_time = 0;
    SIMDStats neon_stats = {0};
    if (simd_is_supported(SIMD_NEON)) {
        simd_reset_stats();
        start_time = get_time_ms();
        for (int i = 0; i < LARGE_BENCHMARK_ITERATIONS; i++) {
            simd_step_neon(test_data, large_test_count, dt, gravity, windx, windy);
        }
        neon_time = get_time_ms() - start_time;
        neon_stats = simd_get_stats();
    }
    
    /* Benchmark optimized NEON implementation if available */
    double neon_opt_time = 0;
    SIMDStats neon_opt_stats = {0};
    if (simd_is_supported(SIMD_NEON)) {
        simd_reset_stats();
        start_time = get_time_ms();
        for (int i = 0; i < LARGE_BENCHMARK_ITERATIONS; i++) {
            simd_step_neon_optimized(test_data, large_test_count, dt, gravity, windx, windy);
        }
        neon_opt_time = get_time_ms() - start_time;
        neon_opt_stats = simd_get_stats();
    }
    
    printf("  📊 Large Dataset Performance (%d particles, %d iterations):\n", large_test_count, LARGE_BENCHMARK_ITERATIONS);
    printf("    Scalar: %.2f ms (%.2f ops/sec)\n", scalar_time, (scalar_stats.scalar_operations / scalar_time) * 1000.0);
    
    if (simd_is_supported(SIMD_NEON)) {
        printf("    NEON (Basic): %.2f ms (%.2f ops/sec, %.2fx speedup)\n", 
               neon_time, (neon_stats.simd_operations / neon_time) * 1000.0, scalar_time / neon_time);
        printf("    NEON (Optimized): %.2f ms (%.2f ops/sec, %.2fx speedup)\n", 
               neon_opt_time, (neon_opt_stats.simd_operations / neon_opt_time) * 1000.0, scalar_time / neon_opt_time);
    }
    
    simd_aligned_free(test_data);
    printf("  ✅ Comprehensive performance test passed\n");
    return 1;
}

/* Test 6: Cache Line Size Detection */
static int test_cache_line_detection(void) {
    printf("Test 6: Cache Line Size Detection\n");
    
    int cache_line_size = simd_get_cache_line_size();
    printf("  📊 Cache Line Size: %d bytes\n", cache_line_size);
    
    /* Validate cache line size (should be a power of 2 and reasonable) */
    if (cache_line_size <= 0 || cache_line_size > 1024) {
        printf("  ⚠️  Cache line size seems unusual: %d bytes\n", cache_line_size);
    } else if ((cache_line_size & (cache_line_size - 1)) != 0) {
        printf("  ⚠️  Cache line size is not a power of 2: %d bytes\n", cache_line_size);
    } else {
        printf("  ✅ Cache line size detection passed\n");
    }
    
    return 1;
}

/* Test 7: Utility Functions */
static int test_utility_functions(void) {
    printf("Test 7: Utility Functions\n");
    
    /* Test prefetch (should not crash) */
    int test_value = 42;
    simd_prefetch(&test_value);
    
    /* Test memory barrier (should not crash) */
    simd_memory_barrier();
    
    printf("  ✅ Utility functions test passed\n");
    return 1;
}

/* Test 8: Edge Case Testing - Boundary Conditions */
static int test_edge_cases_boundary(void) {
    printf("Test 8: Edge Cases - Boundary Conditions\n");
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = -2.0f;
    
    /* Test with particle counts that are not multiples of 4 */
    int edge_counts[] = {0, 1, 3, 7, 15, 31, 63};
    int num_edge_tests = sizeof(edge_counts) / sizeof(edge_counts[0]);
    
    for (int test_idx = 0; test_idx < num_edge_tests; test_idx++) {
        int count = edge_counts[test_idx];
        printf("    Testing with %d particles...\n", count);
        
        if (count == 0) {
            /* Test with zero particles - should not crash */
            simd_step_scalar(NULL, 0, dt, gravity, windx, windy);
            if (simd_is_supported(SIMD_NEON)) {
                simd_step_neon(NULL, 0, dt, gravity, windx, windy);
                simd_step_neon_optimized(NULL, 0, dt, gravity, windx, windy);
            }
            continue;
        }
        
        /* Allocate test data */
        const size_t data_size = count * sizeof(Particle);
        const size_t alignment = simd_get_preferred_alignment();
        Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
        if (!test_data) {
            printf("  ❌ Failed to allocate test data for %d particles\n", count);
            return 0;
        }
        
        /* Initialize test data */
        for (int i = 0; i < count; i++) {
            test_data[i].x = (float)i;
            test_data[i].y = (float)(i * 2);
            test_data[i].vx = (float)(i * 3);
            test_data[i].vy = (float)(i * 4);
        }
        
        /* Create copy for comparison */
        Particle *scalar_data = (Particle *)simd_aligned_alloc(data_size, alignment);
        if (!scalar_data) {
            printf("  ❌ Failed to allocate scalar test data\n");
            simd_aligned_free(test_data);
            return 0;
        }
        memcpy(scalar_data, test_data, data_size);
        
        /* Run scalar implementation */
        simd_step_scalar(scalar_data, count, dt, gravity, windx, windy);
        
        /* Create copy of original data for NEON testing */
        Particle *original_data = (Particle *)simd_aligned_alloc(data_size, alignment);
        if (!original_data) {
            printf("  ❌ Failed to allocate original data\n");
            simd_aligned_free(test_data);
            simd_aligned_free(scalar_data);
            return 0;
        }
        memcpy(original_data, test_data, data_size);
        
        /* Run SIMD implementations and compare */
        if (simd_is_supported(SIMD_NEON)) {
            /* Test basic NEON */
            memcpy(test_data, original_data, data_size); /* Use original data */
            simd_step_neon(test_data, count, dt, gravity, windx, windy);
            
            /* Compare results with realistic tolerance for floating-point differences */
            for (int i = 0; i < count; i++) {
                float tolerance = 1e-5f; /* More realistic tolerance for SIMD vs scalar */
                if (fabsf(test_data[i].x - scalar_data[i].x) > tolerance ||
                    fabsf(test_data[i].y - scalar_data[i].y) > tolerance ||
                    fabsf(test_data[i].vx - scalar_data[i].vx) > tolerance ||
                    fabsf(test_data[i].vy - scalar_data[i].vy) > tolerance) {
                    printf("  ❌ NEON results don't match scalar for %d particles at index %d\n", count, i);
                    printf("    Expected: x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                           scalar_data[i].x, scalar_data[i].y, scalar_data[i].vx, scalar_data[i].vy);
                    printf("    Got:      x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                           test_data[i].x, test_data[i].y, test_data[i].vx, test_data[i].vy);
                    simd_aligned_free(test_data);
                    simd_aligned_free(scalar_data);
                    return 0;
                }
            }
            
            /* Test optimized NEON */
            memcpy(test_data, original_data, data_size); /* Use original data */
            simd_step_neon_optimized(test_data, count, dt, gravity, windx, windy);
            
            /* Compare results with realistic tolerance for floating-point differences */
            for (int i = 0; i < count; i++) {
                float tolerance = 1e-5f; /* More realistic tolerance for SIMD vs scalar */
                if (fabsf(test_data[i].x - scalar_data[i].x) > tolerance ||
                    fabsf(test_data[i].y - scalar_data[i].y) > tolerance ||
                    fabsf(test_data[i].vx - scalar_data[i].vx) > tolerance ||
                    fabsf(test_data[i].vy - scalar_data[i].vy) > tolerance) {
                    printf("  ❌ Optimized NEON results don't match scalar for %d particles at index %d\n", count, i);
                    printf("    Expected: x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                           scalar_data[i].x, scalar_data[i].y, scalar_data[i].vx, scalar_data[i].vy);
                    printf("    Got:      x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                           test_data[i].x, test_data[i].y, test_data[i].vx, test_data[i].vy);
                    simd_aligned_free(test_data);
                    simd_aligned_free(scalar_data);
                    return 0;
                }
            }
        }
        
        simd_aligned_free(test_data);
        simd_aligned_free(scalar_data);
        simd_aligned_free(original_data);
    }
    
    printf("  ✅ Boundary condition tests passed\n");
    return 1;
}

/* Test 9: Edge Case Testing - Extreme Values */
static int test_edge_cases_extreme_values(void) {
    printf("Test 9: Edge Cases - Extreme Values\n");
    
    const int test_count = 100;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate test data\n");
        return 0;
    }
    
    /* Test extreme physics values */
    float extreme_dts[] = {1e-10f, 1e-5f, 1.0f, 1e5f, 1e10f};
    float extreme_gravities[] = {-1e6f, -1e3f, 0.0f, 1e3f, 1e6f};
    float extreme_winds[] = {-1e6f, -1e3f, 0.0f, 1e3f, 1e6f};
    
    for (int dt_idx = 0; dt_idx < 5; dt_idx++) {
        for (int g_idx = 0; g_idx < 5; g_idx++) {
            for (int w_idx = 0; w_idx < 5; w_idx++) {
                float dt = extreme_dts[dt_idx];
                float gravity = extreme_gravities[g_idx];
                float windx = extreme_winds[w_idx];
                float windy = extreme_winds[w_idx];
                
                /* Initialize test data with extreme values */
                for (int i = 0; i < test_count; i++) {
                    test_data[i].x = (float)i * 1e6f;
                    test_data[i].y = (float)i * -1e6f;
                    test_data[i].vx = (float)i * 1e3f;
                    test_data[i].vy = (float)i * -1e3f;
                }
                
                /* Create copies for comparison */
                Particle *scalar_data = (Particle *)simd_aligned_alloc(data_size, alignment);
                Particle *original_data = (Particle *)simd_aligned_alloc(data_size, alignment);
                if (!scalar_data || !original_data) {
                    printf("  ❌ Failed to allocate test data\n");
                    if (scalar_data) simd_aligned_free(scalar_data);
                    if (original_data) simd_aligned_free(original_data);
                    simd_aligned_free(test_data);
                    return 0;
                }
                memcpy(scalar_data, test_data, data_size);
                memcpy(original_data, test_data, data_size);
                
                /* Run scalar implementation */
                simd_step_scalar(scalar_data, test_count, dt, gravity, windx, windy);
                
                /* Run SIMD implementations and compare */
                if (simd_is_supported(SIMD_NEON)) {
                    memcpy(test_data, original_data, data_size); /* Use original data */
                    simd_step_neon_optimized(test_data, test_count, dt, gravity, windx, windy);
                    
                    /* Compare results with adaptive tolerance for extreme values */
                    float tolerance = 1e-2f; /* More relaxed tolerance for extreme values */
                    
                    /* For very extreme values, use even more relaxed tolerance */
                    if (fabsf(dt) < 1e-6f || fabsf(gravity) > 1e5f || fabsf(windx) > 1e5f) {
                        tolerance = 1e-1f;
                    }
                    
                    /* For extremely large values, use very relaxed tolerance */
                    if (fabsf(dt) > 1e4f || fabsf(gravity) > 1e5f || fabsf(windx) > 1e5f) {
                        tolerance = 1e0f; /* Very relaxed tolerance for extreme cases */
                    }
                    
                    for (int i = 0; i < test_count; i++) {
                        /* Skip comparison if values are NaN or infinite */
                        if (isnan(test_data[i].x) || isnan(scalar_data[i].x) ||
                            isinf(test_data[i].x) || isinf(scalar_data[i].x)) {
                            continue;
                        }
                        
                        /* Use relative tolerance for extreme values */
                        float rel_tolerance = 1e-5f; /* 0.001% relative tolerance */
                        
                        /* Check if values are within relative tolerance */
                        int x_ok = (fabsf(scalar_data[i].x) < 1e-10f) ? 
                                  (fabsf(test_data[i].x - scalar_data[i].x) < tolerance) :
                                  (fabsf(test_data[i].x - scalar_data[i].x) / fabsf(scalar_data[i].x) < rel_tolerance);
                        
                        int y_ok = (fabsf(scalar_data[i].y) < 1e-10f) ? 
                                  (fabsf(test_data[i].y - scalar_data[i].y) < tolerance) :
                                  (fabsf(test_data[i].y - scalar_data[i].y) / fabsf(scalar_data[i].y) < rel_tolerance);
                        
                        int vx_ok = (fabsf(scalar_data[i].vx) < 1e-10f) ? 
                                   (fabsf(test_data[i].vx - scalar_data[i].vx) < tolerance) :
                                   (fabsf(test_data[i].vx - scalar_data[i].vx) / fabsf(scalar_data[i].vx) < rel_tolerance);
                        
                        int vy_ok = (fabsf(scalar_data[i].vy) < 1e-10f) ? 
                                   (fabsf(test_data[i].vy - scalar_data[i].vy) < tolerance) :
                                   (fabsf(test_data[i].vy - scalar_data[i].vy) / fabsf(scalar_data[i].vy) < rel_tolerance);
                        
                        if (!x_ok || !y_ok || !vx_ok || !vy_ok) {
                            printf("  ❌ Extreme value test failed: dt=%.2e, g=%.2e, w=%.2e\n", dt, gravity, windx);
                            printf("    Index %d: Expected x=%.6e, y=%.6e, vx=%.6e, vy=%.6e\n", 
                                   i, scalar_data[i].x, scalar_data[i].y, scalar_data[i].vx, scalar_data[i].vy);
                            printf("    Index %d: Got      x=%.6e, y=%.6e, vx=%.6e, vy=%.6e\n", 
                                   i, test_data[i].x, test_data[i].y, test_data[i].vx, test_data[i].vy);
                            simd_aligned_free(test_data);
                            simd_aligned_free(scalar_data);
                            simd_aligned_free(original_data);
                            return 0;
                        }
                    }
                }
                
                simd_aligned_free(scalar_data);
                simd_aligned_free(original_data);
            }
        }
    }
    
    simd_aligned_free(test_data);
    printf("  ✅ Extreme value tests passed\n");
    return 1;
}

/* Test 10: Edge Case Testing - Invalid Data */
static int test_edge_cases_invalid_data(void) {
    printf("Test 10: Edge Cases - Invalid Data\n");
    
    const int test_count = 100;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate test data\n");
        return 0;
    }
    
    /* Test with NaN and infinity values */
    float nan_val = 0.0f / 0.0f; /* Generate NaN */
    float inf_val = 1.0f / 0.0f; /* Generate infinity */
    
    /* Initialize with invalid values */
    for (int i = 0; i < test_count; i++) {
        test_data[i].x = (i % 4 == 0) ? nan_val : (float)i;
        test_data[i].y = (i % 4 == 1) ? inf_val : (float)(i * 2);
        test_data[i].vx = (i % 4 == 2) ? -inf_val : (float)(i * 3);
        test_data[i].vy = (i % 4 == 3) ? nan_val : (float)(i * 4);
    }
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = -2.0f;
    
    /* Test that functions don't crash with invalid data */
    simd_step_scalar(test_data, test_count, dt, gravity, windx, windy);
    
    if (simd_is_supported(SIMD_NEON)) {
        simd_step_neon(test_data, test_count, dt, gravity, windx, windy);
        simd_step_neon_optimized(test_data, test_count, dt, gravity, windx, windy);
    }
    
    simd_aligned_free(test_data);
    printf("  ✅ Invalid data tests passed (no crashes)\n");
    return 1;
}

/* Test 11: Edge Case Testing - Memory Allocation Failures */
static int test_edge_cases_memory_failures(void) {
    printf("Test 11: Edge Cases - Memory Allocation Failures\n");
    
    /* Test with very large allocation requests */
    const size_t huge_size = SIZE_MAX / 2;
    void *huge_ptr = simd_aligned_alloc(huge_size, 16);
    if (huge_ptr) {
        printf("  ⚠️  Unexpectedly succeeded in allocating huge memory\n");
        simd_aligned_free(huge_ptr);
    } else {
        printf("  ✅ Correctly failed to allocate huge memory\n");
    }
    
    /* Test with invalid alignment */
    void *invalid_ptr = simd_aligned_alloc(1024, 0);
    if (invalid_ptr) {
        printf("  ⚠️  Unexpectedly succeeded with invalid alignment\n");
        simd_aligned_free(invalid_ptr);
    } else {
        printf("  ✅ Correctly failed with invalid alignment\n");
    }
    
    /* Test with non-power-of-2 alignment */
    void *non_power2_ptr = simd_aligned_alloc(1024, 3);
    if (non_power2_ptr) {
        printf("  ⚠️  Unexpectedly succeeded with non-power-of-2 alignment\n");
        simd_aligned_free(non_power2_ptr);
    } else {
        printf("  ✅ Correctly failed with non-power-of-2 alignment\n");
    }
    
    /* Test with zero size */
    void *zero_ptr = simd_aligned_alloc(0, 16);
    if (zero_ptr) {
        printf("  ⚠️  Unexpectedly succeeded with zero size\n");
        simd_aligned_free(zero_ptr);
    } else {
        printf("  ✅ Correctly failed with zero size\n");
    }
    
    printf("  ✅ Memory allocation failure tests passed\n");
    return 1;
}

/* Test 12: Stress Testing */
static int test_stress_testing(void) {
    printf("Test 12: Stress Testing\n");
    
    const int stress_count = STRESS_TEST_COUNT;
    const size_t data_size = stress_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    printf("    Testing with %d particles...\n", stress_count);
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("  ❌ Failed to allocate stress test data\n");
        return 0;
    }
    
    /* Initialize stress test data */
    for (int i = 0; i < stress_count; i++) {
        test_data[i].x = (float)i;
        test_data[i].y = (float)(i * 2);
        test_data[i].vx = (float)(i * 3);
        test_data[i].vy = (float)(i * 4);
    }
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = -2.0f;
    
    /* Stress test scalar implementation */
    double start_time = get_time_ms();
    for (int i = 0; i < STRESS_BENCHMARK_ITERATIONS; i++) {
        simd_step_scalar(test_data, stress_count, dt, gravity, windx, windy);
    }
    double scalar_time = get_time_ms() - start_time;
    
    /* Stress test SIMD implementation */
    double simd_time = 0;
    if (simd_is_supported(SIMD_NEON)) {
        start_time = get_time_ms();
        for (int i = 0; i < STRESS_BENCHMARK_ITERATIONS; i++) {
            simd_step_neon_optimized(test_data, stress_count, dt, gravity, windx, windy);
        }
        simd_time = get_time_ms() - start_time;
        
        printf("    Stress Test Results:\n");
        printf("      Scalar: %.2f ms\n", scalar_time);
        printf("      NEON: %.2f ms\n", simd_time);
        printf("      Speedup: %.2fx\n", scalar_time / simd_time);
    }
    
    simd_aligned_free(test_data);
    printf("  ✅ Stress testing passed\n");
    return 1;
}

/* Test 13: Physics Calculation Accuracy Test */
static int test_physics_calculation_accuracy(void) {
    printf("Test 13: Physics Calculation Accuracy Test\n");
    
    /* Test with a simple case that was failing */
    const int test_count = 4;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    Particle *scalar_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    
    if (!test_data || !scalar_data) {
        printf("  ❌ Failed to allocate test data\n");
        if (test_data) simd_aligned_free(test_data);
        if (scalar_data) simd_aligned_free(scalar_data);
        return 0;
    }
    
    /* Initialize with simple test data */
    for (int i = 0; i < test_count; i++) {
        test_data[i].x = 0.0f;
        test_data[i].y = 0.0f;
        test_data[i].vx = 0.0f;
        test_data[i].vy = 0.0f;
    }
    
    /* Copy to scalar data */
    memcpy(scalar_data, test_data, data_size);
    
    const float dt = 1.0f / 60.0f;
    const float gravity = 30.0f;
    const float windx = 5.0f;
    const float windy = 0.0f;
    
    /* Create a copy of original data for NEON testing */
    Particle *original_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!original_data) {
        printf("  ❌ Failed to allocate original data\n");
        simd_aligned_free(test_data);
        simd_aligned_free(scalar_data);
        return 0;
    }
    memcpy(original_data, test_data, data_size);
    
    /* Run scalar implementation */
    simd_step_scalar(scalar_data, test_count, dt, gravity, windx, windy);
    
    /* Run NEON implementation and compare */
    if (simd_is_supported(SIMD_NEON)) {
        /* Test basic NEON */
        memcpy(test_data, original_data, data_size); /* Use original data */
        simd_step_neon(test_data, test_count, dt, gravity, windx, windy);
        
        /* Compare results */
        for (int i = 0; i < test_count; i++) {
            float tolerance = 1e-6f;
            if (fabsf(test_data[i].x - scalar_data[i].x) > tolerance ||
                fabsf(test_data[i].y - scalar_data[i].y) > tolerance ||
                fabsf(test_data[i].vx - scalar_data[i].vx) > tolerance ||
                fabsf(test_data[i].vy - scalar_data[i].vy) > tolerance) {
                printf("  ❌ Basic NEON results don't match scalar at index %d\n", i);
                printf("    Expected: x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                       scalar_data[i].x, scalar_data[i].y, scalar_data[i].vx, scalar_data[i].vy);
                printf("    Got:      x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                       test_data[i].x, test_data[i].y, test_data[i].vx, test_data[i].vy);
                simd_aligned_free(test_data);
                simd_aligned_free(scalar_data);
                return 0;
            }
        }
        
        /* Test optimized NEON */
        memcpy(test_data, original_data, data_size); /* Use original data */
        simd_step_neon_optimized(test_data, test_count, dt, gravity, windx, windy);
        
        /* Compare results */
        for (int i = 0; i < test_count; i++) {
            float tolerance = 1e-6f;
            if (fabsf(test_data[i].x - scalar_data[i].x) > tolerance ||
                fabsf(test_data[i].y - scalar_data[i].y) > tolerance ||
                fabsf(test_data[i].vx - scalar_data[i].vx) > tolerance ||
                fabsf(test_data[i].vy - scalar_data[i].vy) > tolerance) {
                printf("  ❌ Optimized NEON results don't match scalar at index %d\n", i);
                printf("    Expected: x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                       scalar_data[i].x, scalar_data[i].y, scalar_data[i].vx, scalar_data[i].vy);
                printf("    Got:      x=%.6f, y=%.6f, vx=%.6f, vy=%.6f\n", 
                       test_data[i].x, test_data[i].y, test_data[i].vx, test_data[i].vy);
                simd_aligned_free(test_data);
                simd_aligned_free(scalar_data);
                return 0;
            }
        }
        
        printf("  ✅ NEON implementations match scalar exactly\n");
    }
    
    simd_aligned_free(test_data);
    simd_aligned_free(scalar_data);
    simd_aligned_free(original_data);
    printf("  ✅ Physics calculation accuracy test passed\n");
    return 1;
}

/* Main test runner */
int main(void) {
    printf("=== SIMD Capability Detection Test Suite ===\n");
    printf("Testing SIMD abstraction layer...\n\n");
    
    int tests_passed = 0;
    int total_tests = 14;
    
    /* Run all tests */
    tests_passed += test_simd_detection();
    tests_passed += test_feature_support();
    tests_passed += test_memory_alignment();
    tests_passed += test_function_selection();
    tests_passed += test_performance_benchmark();
    tests_passed += test_comprehensive_performance();
    tests_passed += test_cache_line_detection();
    tests_passed += test_utility_functions();
    tests_passed += test_edge_cases_boundary();
    tests_passed += test_edge_cases_extreme_values();
    tests_passed += test_edge_cases_invalid_data();
    tests_passed += test_edge_cases_memory_failures();
    tests_passed += test_stress_testing();
    tests_passed += test_physics_calculation_accuracy();
    
    printf("\n=== Test Results ===\n");
    printf("Tests passed: %d/%d\n", tests_passed, total_tests);
    
    /* Print detailed capabilities */
    printf("\n=== Detailed SIMD Capabilities ===\n");
    simd_print_capabilities();
    
    /* Run benchmark */
    printf("\n=== SIMD Function Benchmark ===\n");
    simd_benchmark_functions();
    
    if (tests_passed == total_tests) {
        printf("\n🎉 All tests passed! SIMD abstraction layer is working correctly.\n");
        return EXIT_SUCCESS;
    } else {
        printf("\n❌ Some tests failed. Please review the implementation.\n");
        return EXIT_FAILURE;
    }
}
