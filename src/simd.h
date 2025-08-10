#ifndef SIMD_H
#define SIMD_H

#include <stdint.h>
#include <stddef.h>

/* SIMD feature flags */
#define SIMD_NONE        0x00000000
#define SIMD_SSE         0x00000001
#define SIMD_SSE2        0x00000002
#define SIMD_SSE3        0x00000004
#define SIMD_SSSE3       0x00000008
#define SIMD_SSE4_1      0x00000010
#define SIMD_SSE4_2      0x00000020
#define SIMD_AVX         0x00000040
#define SIMD_AVX2        0x00000080
#define SIMD_FMA         0x00000100
#define SIMD_AVX512F     0x00000200
#define SIMD_NEON        0x00000400  /* ARM NEON SIMD */

typedef uint32_t SIMDFeature;

/* SIMD capabilities structure */
typedef struct {
    uint32_t features;           /* Supported SIMD features */
    int max_vector_width;        /* Maximum vector width in bytes */
    int preferred_alignment;     /* Preferred memory alignment */
    int cache_line_size;         /* Cache line size in bytes */
} SIMDCapabilities;

/* SIMD statistics structure */
typedef struct {
    size_t simd_operations;      /* Number of SIMD operations performed */
    size_t scalar_operations;    /* Number of scalar operations performed */
    double simd_utilization;     /* Percentage of operations using SIMD */
    double vectorization_efficiency; /* Efficiency of vectorization */
    double alignment_efficiency; /* Efficiency of memory alignment */
} SIMDStats;

/* SIMD step function type */
typedef void (*simd_step_func_t)(void *particles, int count, float dt, float gravity, float windx, float windy);

/* Core SIMD functions */
SIMDCapabilities simd_detect_capabilities(void);
uint32_t simd_get_supported_features(void);
int simd_is_supported(SIMDFeature feature);
int simd_get_max_vector_width(void);
int simd_get_preferred_alignment(void);

/* Memory management */
void *simd_aligned_alloc(size_t size, size_t alignment);
void simd_aligned_free(void *ptr);
int simd_is_aligned(const void *ptr, size_t alignment);
size_t simd_align_size(size_t size, size_t alignment);

/* Function selection */
simd_step_func_t simd_select_step_function(void);
const char *simd_get_function_name(simd_step_func_t func);

/* Performance monitoring */
SIMDStats simd_get_stats(void);
void simd_reset_stats(void);
void simd_print_stats(void);

/* Utility functions */
int simd_get_cache_line_size(void);
void simd_prefetch(const void *ptr);
void simd_memory_barrier(void);

/* Debug and testing */
void simd_print_capabilities(void);
int simd_validate_alignment(const void *ptr, size_t size, size_t alignment);
void simd_benchmark_functions(void);

/* SIMD step function implementations */
void simd_step_scalar(void *particles, int count, float dt, float gravity, float windx, float windy);
void simd_step_sse(void *particles, int count, float dt, float gravity, float windx, float windy);
void simd_step_avx(void *particles, int count, float dt, float gravity, float windx, float windy);
void simd_step_neon(void *particles, int count, float dt, float gravity, float windx, float windy);
void simd_step_neon_optimized(void *particles, int count, float dt, float gravity, float windx, float windy);

#endif /* SIMD_H */
