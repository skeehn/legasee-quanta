#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Error statistics */
static ErrorStats g_error_stats = {0};

/* Error code to string mapping */
static const char *ERROR_CODE_STRINGS[] = {
    "SUCCESS",
    "MEMORY_ALLOCATION",
    "INVALID_PARAMETER", 
    "NULL_POINTER",
    "OUT_OF_RANGE",
    "OUT_OF_RESOURCES",
    "SYSTEM_ERROR",
    "USER_REQUESTED_EXIT",
    "UNKNOWN"
};

/* Initialize error handling system */
void error_init(void) {
    error_reset_stats();
}

/* Cleanup error handling system */
void error_cleanup(void) {
    if (g_error_stats.total_errors > 0) {
        error_print_stats();
    }
}

/* Convert error code to string */
const char* error_code_to_string(ErrorCode code) {
    if (code <= ERROR_UNKNOWN) {
        return ERROR_CODE_STRINGS[code];
    }
    return "INVALID_ERROR_CODE";
}

/* Get error description */
const char* error_get_description(const Error *error) {
    if (!error) return "NULL_ERROR";
    return error->message ? error->message : "No description";
}

/* Print error to stderr */
void error_print(const Error *error) {
    if (!error) return;
    
    time_t now = time(NULL);
    char time_str[26];
    ctime_r(&now, time_str);
    time_str[24] = '\0'; /* Remove newline */
    
    fprintf(stderr, "[%s] ERROR: %s (%s)\n", 
            time_str, 
            error_code_to_string(error->code),
            error_get_description(error));
    
    if (error->file && error->line > 0) {
        fprintf(stderr, "  Location: %s:%d in %s()\n", 
                error->file, error->line, error->function ? error->function : "unknown");
    }
    
    /* Update statistics */
    g_error_stats.total_errors++;
    if (error->code == ERROR_NULL_POINTER || error->code == ERROR_MEMORY_ALLOCATION) {
        g_error_stats.memory_errors++;
    } else {
        g_error_stats.parameter_errors++;
    }
}

/* Memory allocation with error handling */
void* error_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        g_error_stats.memory_allocation_failures++;
    } else {
        g_error_stats.memory_allocations++;
    }
    return ptr;
}

/* Memory allocation with error handling (calloc) */
void* error_calloc(size_t nmemb, size_t size) {
    void *ptr = calloc(nmemb, size);
    if (!ptr) {
        g_error_stats.memory_allocation_failures++;
    } else {
        g_error_stats.memory_allocations++;
    }
    return ptr;
}

void error_free(void *ptr) {
    if (ptr) {
        free(ptr);
        g_error_stats.memory_deallocations++;
    }
}

/* Null pointer checking */
Error error_check_null(const void *ptr, const char *name) {
    if (!ptr) {
        char message[256];
        snprintf(message, sizeof(message), "NULL pointer: %s", name ? name : "unknown");
        return ERROR_CREATE(ERROR_NULL_POINTER, message);
    }
    return (Error){SUCCESS, NULL, __FILE__, __LINE__, __func__};
}

/* Range validation */
Error error_check_range(int value, int min, int max, const char *name) {
    if (value < min || value > max) {
        char message[256];
        snprintf(message, sizeof(message), "Value out of range: %s=%d (expected %d-%d)", 
                name ? name : "value", value, min, max);
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, message);
    }
    return (Error){SUCCESS, NULL, __FILE__, __LINE__, __func__};
}

/* Get error statistics */
ErrorStats error_get_stats(void) {
    return g_error_stats;
}

/* Reset error statistics */
void error_reset_stats(void) {
    memset(&g_error_stats, 0, sizeof(ErrorStats));
}

/* Print error statistics */
void error_print_stats(void) {
    if (g_error_stats.total_errors == 0) {
        printf("No errors recorded.\n");
        return;
    }
    
    printf("\n=== Error Statistics ===\n");
    printf("Total Errors: %zu\n", g_error_stats.total_errors);
    printf("Memory Errors: %zu\n", g_error_stats.memory_errors);
    printf("Parameter Errors: %zu\n", g_error_stats.parameter_errors);
    printf("=======================\n\n");
}
