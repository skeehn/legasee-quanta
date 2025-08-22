#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

/* Error codes for consistent error handling across the codebase */
typedef enum {
    SUCCESS = 0,
    ERROR_NULL_POINTER,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_PARAMETER,
    ERROR_OUT_OF_RANGE,
    ERROR_INVALID_STATE,
    ERROR_UNKNOWN
} ErrorCode;

/* Error structure with detailed information */
typedef struct {
    ErrorCode code;
    const char *message;
    const char *file;
    int line;
    const char *function;
} Error;

/* Error creation macros */
#define ERROR_CREATE(code, msg) \
    (Error){code, msg, __FILE__, __LINE__, __func__}

/* Error checking macros */
#define ERROR_CHECK(expr) \
    do { \
        Error _err = (expr); \
        if (_err.code != SUCCESS) { \
            return _err; \
        } \
    } while(0)

/* Core error handling functions */
void error_init(void);
void error_cleanup(void);

/* Error message utilities */
const char* error_code_to_string(ErrorCode code);
const char* error_get_description(const Error *error);
void error_print(const Error *error);

/* Memory allocation with error handling */
void* error_malloc(size_t size);
void error_free(void *ptr);

/* Null pointer checking */
Error error_check_null(const void *ptr, const char *name);

/* Range validation */
Error error_check_range(int value, int min, int max, const char *name);

/* Error statistics */
typedef struct {
    size_t total_errors;
    size_t memory_errors;
    size_t parameter_errors;
} ErrorStats;

ErrorStats error_get_stats(void);
void error_reset_stats(void);
void error_print_stats(void);

#endif /* ERROR_H */
