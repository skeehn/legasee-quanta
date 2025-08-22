#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../src/error.h"

/* Test function type */
typedef Error (*TestFunction)(void);

/* Test function declarations */
static Error test_error_creation(void);
static Error test_null_pointer_checking(void);
static Error test_range_validation(void);
static Error test_memory_allocation(void);
static Error test_error_statistics(void);

/* Test utilities */
static void test_print_result(const char *test_name, Error error);
static double test_get_time_ms(void);

int main(void) {
    printf("=== ASCII Particle Simulator - Error System Test ===\n\n");
    
    /* Initialize error system */
    printf("Initializing error handling system...\n");
    error_init();
    printf("Error system initialized!\n\n");
    
    /* Define tests */
    TestFunction tests[] = {
        test_error_creation,
        test_null_pointer_checking,
        test_range_validation,
        test_memory_allocation,
        test_error_statistics
    };
    
    const char *test_names[] = {
        "Error Creation",
        "Null Pointer Checking",
        "Range Validation",
        "Memory Allocation",
        "Error Statistics"
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    int passed_tests = 0;
    int failed_tests = 0;
    double total_time = 0.0;
    
    /* Run tests */
    printf("Running tests...\n\n");
    
    for (int i = 0; i < num_tests; i++) {
        printf("Test %d: %s\n", i + 1, test_names[i]);
        double start_time = test_get_time_ms();
        
        Error test_error = tests[i]();
        
        double end_time = test_get_time_ms();
        double duration = end_time - start_time;
        total_time += duration;
        
        test_print_result(test_names[i], test_error);
        
        if (test_error.code == SUCCESS) {
            passed_tests++;
            printf("  PASSED (%.2f ms)\n", duration);
        } else {
            failed_tests++;
            printf("  FAILED (%.2f ms)\n", duration);
        }
        printf("\n");
    }
    
    /* Print summary */
    printf("=== Test Results ===\n");
    printf("Total Tests: %d\n", num_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", failed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / num_tests * 100.0f);
    printf("Total Time: %.2f ms\n", total_time);
    printf("==================\n\n");
    
    /* Print error statistics */
    printf("=== Error Statistics ===\n");
    error_print_stats();
    
    /* Cleanup */
    error_cleanup();
    
    return failed_tests > 0 ? 1 : 0;
}

/* Test error creation */
static Error test_error_creation(void) {
    printf("  Testing error creation...\n");
    
    /* Test basic error creation */
    Error err = ERROR_CREATE(ERROR_NULL_POINTER, "Test null pointer error");
    if (err.code != ERROR_NULL_POINTER) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Error creation failed");
    }
    
    /* Test error message */
    if (!err.message || strcmp(err.message, "Test null pointer error") != 0) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Error message not set correctly");
    }
    
    /* Test error location */
    if (!err.file || !err.function) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Error location not set correctly");
    }
    
    return (Error){SUCCESS};
}

/* Test null pointer checking */
static Error test_null_pointer_checking(void) {
    printf("  Testing null pointer validation...\n");
    
    /* Test with NULL pointer */
    void *ptr = NULL;
    Error null_check = error_check_null(ptr, "test_pointer");
    if (null_check.code != ERROR_NULL_POINTER) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Null pointer check failed");
    }
    
    /* Test with valid pointer */
    int value = 42;
    Error valid_check = error_check_null(&value, "test_pointer");
    if (valid_check.code != SUCCESS) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Valid pointer check failed");
    }
    
    return (Error){SUCCESS};
}

/* Test range validation */
static Error test_range_validation(void) {
    printf("  Testing range validation...\n");
    
    /* Test value in range */
    Error in_range = error_check_range(50, 1, 100, "test_value");
    if (in_range.code != SUCCESS) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "In-range value check failed");
    }
    
    /* Test value below range */
    Error below_range = error_check_range(0, 1, 100, "test_value");
    if (below_range.code != ERROR_OUT_OF_RANGE) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Below-range value check failed");
    }
    
    /* Test value above range */
    Error above_range = error_check_range(150, 1, 100, "test_value");
    if (above_range.code != ERROR_OUT_OF_RANGE) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Above-range value check failed");
    }
    
    /* Test boundary values */
    Error min_boundary = error_check_range(1, 1, 100, "test_value");
    if (min_boundary.code != SUCCESS) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Minimum boundary check failed");
    }
    
    Error max_boundary = error_check_range(100, 1, 100, "test_value");
    if (max_boundary.code != SUCCESS) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Maximum boundary check failed");
    }
    
    return (Error){SUCCESS};
}

/* Test memory allocation */
static Error test_memory_allocation(void) {
    printf("  Testing memory allocation...\n");
    
    /* Test valid allocation */
    void *mem = error_malloc(1024);
    if (!mem) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Valid memory allocation failed");
    }
    error_free(mem);
    
    /* Test zero size allocation */
    void *zero_mem = error_malloc(0);
    if (zero_mem) {
        error_free(zero_mem);
        return ERROR_CREATE(ERROR_INVALID_STATE, "Zero size allocation should have failed");
    }
    
    /* Test large allocation (should succeed on most systems) */
    void *large_mem = error_malloc(1024 * 1024); /* 1MB */
    if (large_mem) {
        error_free(large_mem);
    }
    /* Large allocation might fail on some systems, so we don't treat it as an error */
    
    return (Error){SUCCESS};
}

/* Test error statistics */
static Error test_error_statistics(void) {
    printf("  Testing error statistics...\n");
    
    /* Reset statistics */
    error_reset_stats();
    
    /* Generate some errors */
    error_check_null(NULL, "test");
    error_check_range(150, 1, 100, "test");
    error_malloc(0);
    
    /* Check statistics */
    ErrorStats stats = error_get_stats();
    if (stats.total_errors == 0) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Error statistics not working");
    }
    
    printf("    Total errors: %zu\n", stats.total_errors);
    printf("    Memory errors: %zu\n", stats.memory_errors);
    printf("    Parameter errors: %zu\n", stats.parameter_errors);
    
    return (Error){SUCCESS};
}

/* Test utilities */
static void test_print_result(const char *test_name, Error error) {
    if (error.code == SUCCESS) {
        printf("  ✓ %s: PASSED\n", test_name);
    } else {
        printf("  ✗ %s: FAILED\n", test_name);
        printf("    Error: %s - %s\n", 
               error_code_to_string(error.code),
               error_get_description(&error));
    }
}

static double test_get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}
