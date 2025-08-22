#ifndef ERROR_H
#define ERROR_H

#include <stddef.h>

/* Error codes for consistent error handling across the codebase */
typedef enum {
    SUCCESS = 0,
    ERROR_MEMORY_ALLOCATION,
    ERROR_INVALID_PARAMETER,
    ERROR_NULL_POINTER,
    ERROR_OUT_OF_RANGE,
    ERROR_OUT_OF_RESOURCES,
    ERROR_SYSTEM_ERROR,
    ERROR_USER_REQUESTED_EXIT,
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
const char* error_code_to_string(ErrorCode code);
void error_print(const Error *error);

/* Error-aware memory management */
void* error_malloc(size_t size);
void *error_calloc(size_t nmemb, size_t size);
void error_free(void *ptr);

/* Error validation helpers */
int error_check_null(const void *ptr, const char *name);
int error_check_range(int value, int min, int max, const char *name);

/* Error statistics structure */
typedef struct {
    size_t total_errors;
    size_t memory_errors;
    size_t parameter_errors;
    size_t null_pointer_errors;
    size_t range_errors;
    size_t resource_errors;
    size_t system_errors;
    size_t unknown_errors;
} ErrorStats;

/* Error statistics functions */
ErrorStats error_get_stats(void);
void error_reset_stats(void);
void error_print_stats(void);

/* Validation macros for common error patterns */
#define ERROR_CHECK_NULL(ptr, name) \
    do { \
        if ((ptr) == NULL) { \
            return ERROR_CREATE(ERROR_NULL_POINTER, name " cannot be NULL"); \
        } \
    } while(0)

#define ERROR_CHECK_RANGE(value, min, max, name) \
    do { \
        if ((value) < (min) || (value) > (max)) { \
            return ERROR_CREATE(ERROR_OUT_OF_RANGE, name " out of range"); \
        } \
    } while(0)

#define ERROR_CHECK_CONDITION(condition, error_code, message) \
    do { \
        if (!(condition)) { \
            return ERROR_CREATE(error_code, message); \
        } \
    } while(0)

void *error_calloc(size_t nmemb, size_t size);

#endif /* ERROR_H */