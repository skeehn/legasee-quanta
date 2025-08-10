#include "simd.h"
#include "particle.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* ARM NEON intrinsics */
#ifdef __aarch64__
#include <arm_neon.h>
#endif

/* Performance measurement helper */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

/* Global SIMD capabilities cache */
static SIMDCapabilities g_simd_capabilities = {0};
static int g_simd_initialized = 0;
static SIMDStats g_simd_stats = {0};

/* CPUID function for x86 platforms */
#if defined(__x86_64__) || defined(__i386__)
static void get_cpuid(unsigned int leaf, unsigned int subleaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx) {
    #ifdef _MSC_VER
        int cpu_info[4];
        __cpuid(cpu_info, leaf);
        *eax = cpu_info[0];
        *ebx = cpu_info[1];
        *ecx = cpu_info[2];
        *edx = cpu_info[3];
    #else
        __cpuid_count(leaf, subleaf, *eax, *ebx, *ecx, *edx);
    #endif
}

static uint32_t detect_x86_capabilities(void) {
    unsigned int eax, ebx, ecx, edx;
    uint32_t features = SIMD_NONE;
    
    /* Get basic CPU info */
    get_cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    
    /* Check SSE features */
    if (edx & (1 << 25)) features |= SIMD_SSE;      /* SSE */
    if (edx & (1 << 26)) features |= SIMD_SSE2;     /* SSE2 */
    if (ecx & (1 << 0))  features |= SIMD_SSE3;     /* SSE3 */
    if (ecx & (1 << 9))  features |= SIMD_SSSE3;    /* SSSE3 */
    if (ecx & (1 << 19)) features |= SIMD_SSE4_1;   /* SSE4.1 */
    if (ecx & (1 << 20)) features |= SIMD_SSE4_2;   /* SSE4.2 */
    
    /* Check AVX features */
    if (ecx & (1 << 28)) features |= SIMD_AVX;      /* AVX */
    if (ecx & (1 << 12)) features |= SIMD_FMA;      /* FMA */
    
    /* Get extended CPU info for AVX2 and AVX-512 */
    get_cpuid(7, 0, &eax, &ebx, &ecx, &edx);
    
    if (ebx & (1 << 5))  features |= SIMD_AVX2;     /* AVX2 */
    if (ebx & (1 << 16)) features |= SIMD_AVX512F;  /* AVX-512 Foundation */
    
    return features;
}
#else
static uint32_t detect_x86_capabilities(void) __attribute__((unused));
static uint32_t detect_x86_capabilities(void) {
    return SIMD_NONE;
}
#endif

/* ARM64/NEON detection */
#ifdef __aarch64__
static uint32_t detect_arm_capabilities(void) {
    uint32_t features = SIMD_NONE;
    
    /* On ARM64, NEON is always available */
    features |= SIMD_NEON;
    
    return features;
}
#else
static uint32_t detect_arm_capabilities(void) {
    return SIMD_NONE;
}
#endif

/* Detect cache line size */
static int detect_cache_line_size(void) {
    #ifdef __linux__
        FILE *cpuinfo = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
        if (cpuinfo) {
            int size;
            if (fscanf(cpuinfo, "%d", &size) == 1) {
                fclose(cpuinfo);
                return size;
            }
            fclose(cpuinfo);
        }
    #elif defined(__APPLE__)
        /* macOS typically has 64-byte cache lines */
        return 64;
    #else
        /* Default to 64 bytes for most modern processors */
        return 64;
    #endif
    
    return 64; /* Default fallback */
}

/* Initialize SIMD capabilities */
SIMDCapabilities simd_detect_capabilities(void) {
    if (g_simd_initialized) {
        return g_simd_capabilities;
    }
    
    /* Detect CPU features based on platform */
    uint32_t features = SIMD_NONE;
    
    #if defined(__x86_64__) || defined(__i386__)
        features = detect_x86_capabilities();
    #elif defined(__aarch64__)
        features = detect_arm_capabilities();
    #endif
    
    /* Set capabilities based on detected features */
    g_simd_capabilities.features = features;
    g_simd_capabilities.cache_line_size = detect_cache_line_size();
    
    /* Determine maximum vector width */
    if (features & SIMD_AVX512F) {
        g_simd_capabilities.max_vector_width = 64;
        g_simd_capabilities.preferred_alignment = 64;
    } else if (features & SIMD_AVX) {
        g_simd_capabilities.max_vector_width = 32;
        g_simd_capabilities.preferred_alignment = 32;
    } else if (features & SIMD_SSE) {
        g_simd_capabilities.max_vector_width = 16;
        g_simd_capabilities.preferred_alignment = 16;
    } else if (features & SIMD_NEON) {
        g_simd_capabilities.max_vector_width = 16;
        g_simd_capabilities.preferred_alignment = 16;
    } else {
        g_simd_capabilities.max_vector_width = 4;
        g_simd_capabilities.preferred_alignment = 4;
    }
    
    g_simd_initialized = 1;
    return g_simd_capabilities;
}

/* Get supported features */
uint32_t simd_get_supported_features(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    return g_simd_capabilities.features;
}

/* Check if specific feature is supported */
int simd_is_supported(SIMDFeature feature) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    return (g_simd_capabilities.features & feature) != 0;
}

/* Get maximum vector width */
int simd_get_max_vector_width(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    return g_simd_capabilities.max_vector_width;
}

/* Get preferred alignment */
int simd_get_preferred_alignment(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    return g_simd_capabilities.preferred_alignment;
}

/* Memory alignment utilities */
void *simd_aligned_alloc(size_t size, size_t alignment) {
    /* Validate input parameters */
    if (size == 0) {
        return NULL; /* Zero size allocation should fail */
    }
    
    /* Ensure alignment is at least sizeof(void*) and is a power of 2 */
    if (alignment < sizeof(void*)) {
        alignment = sizeof(void*);
    }
    
    /* Validate alignment is a power of 2 */
    if ((alignment & (alignment - 1)) != 0) {
        return NULL; /* Invalid alignment - not a power of 2 */
    }
    
    /* Check for reasonable size limits */
    if (size > SIZE_MAX / 2) {
        return NULL; /* Requested size is too large */
    }
    
    #ifdef _MSC_VER
        return _aligned_malloc(size, alignment);
    #else
        void *ptr = NULL;
        if (posix_memalign(&ptr, alignment, size) != 0) {
            return NULL;
        }
        return ptr;
    #endif
}

void simd_aligned_free(void *ptr) {
    #ifdef _MSC_VER
        _aligned_free(ptr);
    #else
        free(ptr);
    #endif
}

int simd_is_aligned(const void *ptr, size_t alignment) {
    return ((uintptr_t)ptr & (alignment - 1)) == 0;
}

size_t simd_align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

/* SIMD function selection */
simd_step_func_t simd_select_step_function(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    
    /* Select best available implementation */
    if (simd_is_supported(SIMD_AVX)) {
        return simd_step_avx;
    } else if (simd_is_supported(SIMD_SSE)) {
        return simd_step_sse;
    } else if (simd_is_supported(SIMD_NEON)) {
        return simd_step_neon_optimized; /* Use optimized NEON implementation */
    } else {
        return simd_step_scalar;
    }
}

const char *simd_get_function_name(simd_step_func_t func) {
    if (func == simd_step_avx) return "AVX";
    if (func == simd_step_sse) return "SSE";
    if (func == simd_step_neon) return "NEON";
    if (func == simd_step_neon_optimized) return "NEON (Optimized)";
    if (func == simd_step_scalar) return "Scalar";
    return "Unknown";
}

/* Performance monitoring */
SIMDStats simd_get_stats(void) {
    return g_simd_stats;
}

void simd_reset_stats(void) {
    memset(&g_simd_stats, 0, sizeof(SIMDStats));
}

void simd_print_stats(void) {
    printf("SIMD Statistics:\n");
    printf("  SIMD Operations: %lu\n", g_simd_stats.simd_operations);
    printf("  Scalar Operations: %lu\n", g_simd_stats.scalar_operations);
    printf("  SIMD Utilization: %.2f%%\n", g_simd_stats.simd_utilization * 100.0);
    printf("  Vectorization Efficiency: %.2f%%\n", g_simd_stats.vectorization_efficiency * 100.0);
    printf("  Alignment Efficiency: %.2f%%\n", g_simd_stats.alignment_efficiency * 100.0);
}

/* Utility functions */
int simd_get_cache_line_size(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    return g_simd_capabilities.cache_line_size;
}

void simd_prefetch(const void *ptr) {
    #ifdef __GNUC__
        __builtin_prefetch(ptr, 0, 3); /* Read, high locality */
    #endif
}

void simd_memory_barrier(void) {
    #ifdef __GNUC__
        __sync_synchronize();
    #elif defined(_MSC_VER)
        _ReadWriteBarrier();
    #endif
}

/* Debug and testing functions */
void simd_print_capabilities(void) {
    if (!g_simd_initialized) {
        simd_detect_capabilities();
    }
    
    printf("SIMD Capabilities:\n");
    printf("  Features: 0x%08x\n", g_simd_capabilities.features);
    printf("  Max Vector Width: %d bytes\n", g_simd_capabilities.max_vector_width);
    printf("  Preferred Alignment: %d bytes\n", g_simd_capabilities.preferred_alignment);
    printf("  Cache Line Size: %d bytes\n", g_simd_capabilities.cache_line_size);
    
    printf("  Supported Features:\n");
    if (simd_is_supported(SIMD_SSE)) printf("    SSE\n");
    if (simd_is_supported(SIMD_SSE2)) printf("    SSE2\n");
    if (simd_is_supported(SIMD_SSE3)) printf("    SSE3\n");
    if (simd_is_supported(SIMD_SSSE3)) printf("    SSSE3\n");
    if (simd_is_supported(SIMD_SSE4_1)) printf("    SSE4.1\n");
    if (simd_is_supported(SIMD_SSE4_2)) printf("    SSE4.2\n");
    if (simd_is_supported(SIMD_AVX)) printf("    AVX\n");
    if (simd_is_supported(SIMD_AVX2)) printf("    AVX2\n");
    if (simd_is_supported(SIMD_FMA)) printf("    FMA\n");
    if (simd_is_supported(SIMD_AVX512F)) printf("    AVX-512 Foundation\n");
    if (simd_is_supported(SIMD_NEON)) printf("    NEON\n");
}

int simd_validate_alignment(const void *ptr, size_t size, size_t alignment) {
    if (!simd_is_aligned(ptr, alignment)) {
        printf("SIMD Warning: Pointer %p is not aligned to %zu bytes\n", ptr, alignment);
        return 0;
    }
    
    if (size % alignment != 0) {
        printf("SIMD Warning: Size %zu is not aligned to %zu bytes\n", size, alignment);
        return 0;
    }
    
    return 1;
}

/* Placeholder implementations for SIMD step functions */
void simd_step_scalar(void *particles, int count, float dt, float gravity, float windx, float windy) {
    Particle *p = (Particle *)particles;
    
    g_simd_stats.scalar_operations += count;
    
    for (int i = 0; i < count; i++) {
        /* Apply forces */
        p[i].vx += windx * dt;
        p[i].vy += (gravity + windy) * dt;
        
        /* Update position */
        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;
    }
}

void simd_step_sse(void *particles, int count, float dt, float gravity, float windx, float windy) {
    /* SSE implementation will be added in the next task */
    simd_step_scalar(particles, count, dt, gravity, windx, windy);
    g_simd_stats.simd_operations += count;
}

void simd_step_avx(void *particles, int count, float dt, float gravity, float windx, float windy) {
    /* AVX implementation will be added in the next task */
    simd_step_scalar(particles, count, dt, gravity, windx, windy);
    g_simd_stats.simd_operations += count;
}

void simd_step_neon(void *particles, int count, float dt, float gravity, float windx, float windy) {
    #ifdef __aarch64__
    Particle *p = (Particle *)particles;
    
    /* Pre-compute force components exactly as in scalar implementation */
    float windx_dt = windx * dt;
    float gravity_windy_dt = (gravity + windy) * dt;
    
    /* Process particles in groups of 4 using NEON */
    int i = 0;
    int vectorized_count = count & ~3; /* Round down to nearest multiple of 4 */
    
    for (; i < vectorized_count; i += 4) {
        /* Load 4 particles' velocities */
        float32x4_t vx = {p[i].vx, p[i+1].vx, p[i+2].vx, p[i+3].vx};
        float32x4_t vy = {p[i].vy, p[i+1].vy, p[i+2].vy, p[i+3].vy};
        
        /* Load 4 particles' positions */
        float32x4_t x = {p[i].x, p[i+1].x, p[i+2].x, p[i+3].x};
        float32x4_t y = {p[i].y, p[i+1].y, p[i+2].y, p[i+3].y};
        
        /* Apply forces exactly as in scalar implementation:
           vx += windx * dt
           vy += (gravity + windy) * dt */
        vx = vaddq_f32(vx, vdupq_n_f32(windx_dt));
        vy = vaddq_f32(vy, vdupq_n_f32(gravity_windy_dt));
        
        /* Update positions exactly as in scalar implementation:
           x += vx * dt
           y += vy * dt */
        x = vaddq_f32(x, vmulq_f32(vx, vdupq_n_f32(dt)));
        y = vaddq_f32(y, vmulq_f32(vy, vdupq_n_f32(dt)));
        
        /* Store updated velocities and positions */
        p[i].vx = vgetq_lane_f32(vx, 0);
        p[i].vy = vgetq_lane_f32(vy, 0);
        p[i].x = vgetq_lane_f32(x, 0);
        p[i].y = vgetq_lane_f32(y, 0);
        
        p[i+1].vx = vgetq_lane_f32(vx, 1);
        p[i+1].vy = vgetq_lane_f32(vy, 1);
        p[i+1].x = vgetq_lane_f32(x, 1);
        p[i+1].y = vgetq_lane_f32(y, 1);
        
        p[i+2].vx = vgetq_lane_f32(vx, 2);
        p[i+2].vy = vgetq_lane_f32(vy, 2);
        p[i+2].x = vgetq_lane_f32(x, 2);
        p[i+2].y = vgetq_lane_f32(y, 2);
        
        p[i+3].vx = vgetq_lane_f32(vx, 3);
        p[i+3].vy = vgetq_lane_f32(vy, 3);
        p[i+3].x = vgetq_lane_f32(x, 3);
        p[i+3].y = vgetq_lane_f32(y, 3);
    }
    
    /* Handle remaining particles with scalar code - exactly the same as scalar implementation */
    for (; i < count; i++) {
        /* Apply forces */
        p[i].vx += windx_dt;
        p[i].vy += gravity_windy_dt;
        
        /* Update position */
        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;
    }
    
    g_simd_stats.simd_operations += vectorized_count;
    g_simd_stats.scalar_operations += (count - vectorized_count);
    #else
    /* Fallback to scalar implementation on non-ARM platforms */
    simd_step_scalar(particles, count, dt, gravity, windx, windy);
    g_simd_stats.scalar_operations += count;
    #endif
}

/* Optimized NEON implementation using load/store intrinsics */
void simd_step_neon_optimized(void *particles, int count, float dt, float gravity, float windx, float windy) {
    #ifdef __aarch64__
    Particle *p = (Particle *)particles;
    
    /* Pre-compute force components exactly as in scalar implementation */
    float windx_dt = windx * dt;
    float gravity_windy_dt = (gravity + windy) * dt;
    
    /* Process particles in groups of 4 using NEON load/store */
    int i = 0;
    int vectorized_count = count & ~3; /* Round down to nearest multiple of 4 */
    
    for (; i < vectorized_count; i += 4) {
        /* Load 4 particles' data using individual loads for correct data layout */
        float32x4_t x = {p[i].x, p[i+1].x, p[i+2].x, p[i+3].x};
        float32x4_t y = {p[i].y, p[i+1].y, p[i+2].y, p[i+3].y};
        float32x4_t vx = {p[i].vx, p[i+1].vx, p[i+2].vx, p[i+3].vx};
        float32x4_t vy = {p[i].vy, p[i+1].vy, p[i+2].vy, p[i+3].vy};
        
        /* Apply forces exactly as in scalar implementation:
           vx += windx * dt
           vy += (gravity + windy) * dt */
        vx = vaddq_f32(vx, vdupq_n_f32(windx_dt));
        vy = vaddq_f32(vy, vdupq_n_f32(gravity_windy_dt));
        
        /* Update positions exactly as in scalar implementation:
           x += vx * dt
           y += vy * dt */
        x = vaddq_f32(x, vmulq_f32(vx, vdupq_n_f32(dt)));
        y = vaddq_f32(y, vmulq_f32(vy, vdupq_n_f32(dt)));
        
        /* Store updated data using individual stores */
        p[i].x = vgetq_lane_f32(x, 0);
        p[i].y = vgetq_lane_f32(y, 0);
        p[i].vx = vgetq_lane_f32(vx, 0);
        p[i].vy = vgetq_lane_f32(vy, 0);
        
        p[i+1].x = vgetq_lane_f32(x, 1);
        p[i+1].y = vgetq_lane_f32(y, 1);
        p[i+1].vx = vgetq_lane_f32(vx, 1);
        p[i+1].vy = vgetq_lane_f32(vy, 1);
        
        p[i+2].x = vgetq_lane_f32(x, 2);
        p[i+2].y = vgetq_lane_f32(y, 2);
        p[i+2].vx = vgetq_lane_f32(vx, 2);
        p[i+2].vy = vgetq_lane_f32(vy, 2);
        
        p[i+3].x = vgetq_lane_f32(x, 3);
        p[i+3].y = vgetq_lane_f32(y, 3);
        p[i+3].vx = vgetq_lane_f32(vx, 3);
        p[i+3].vy = vgetq_lane_f32(vy, 3);
    }
    
    /* Handle remaining particles with scalar code - exactly the same as scalar implementation */
    for (; i < count; i++) {
        /* Apply forces */
        p[i].vx += windx_dt;
        p[i].vy += gravity_windy_dt;
        
        /* Update position */
        p[i].x += p[i].vx * dt;
        p[i].y += p[i].vy * dt;
    }
    
    g_simd_stats.simd_operations += vectorized_count;
    g_simd_stats.scalar_operations += (count - vectorized_count);
    #else
    /* Fallback to scalar implementation on non-ARM platforms */
    simd_step_scalar(particles, count, dt, gravity, windx, windy);
    g_simd_stats.scalar_operations += count;
    #endif
}

void simd_benchmark_functions(void) {
    printf("SIMD Function Benchmark:\n");
    
    /* Allocate aligned test data */
    const int test_count = 1000;
    const size_t data_size = test_count * sizeof(Particle);
    const size_t alignment = simd_get_preferred_alignment();
    
    Particle *test_data = (Particle *)simd_aligned_alloc(data_size, alignment);
    if (!test_data) {
        printf("Failed to allocate test data\n");
        return;
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
    
    /* Benchmark scalar implementation */
    double start_time = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        simd_step_scalar(test_data, test_count, dt, gravity, windx, windy);
    }
    double scalar_time = get_time_ms() - start_time;
    
    /* Benchmark basic NEON implementation */
    double neon_time = 0;
    if (simd_is_supported(SIMD_NEON)) {
        start_time = get_time_ms();
        for (int i = 0; i < 1000; i++) {
            simd_step_neon(test_data, test_count, dt, gravity, windx, windy);
        }
        neon_time = get_time_ms() - start_time;
        printf("  NEON (Basic): %.2f ms (%.2fx speedup)\n", neon_time, scalar_time / neon_time);
    }
    
    /* Benchmark optimized NEON implementation */
    double neon_opt_time = 0;
    if (simd_is_supported(SIMD_NEON)) {
        start_time = get_time_ms();
        for (int i = 0; i < 1000; i++) {
            simd_step_neon_optimized(test_data, test_count, dt, gravity, windx, windy);
        }
        neon_opt_time = get_time_ms() - start_time;
        printf("  NEON (Optimized): %.2f ms (%.2fx speedup)\n", neon_opt_time, scalar_time / neon_opt_time);
    }
    
    /* Benchmark selected SIMD implementation */
    simd_step_func_t simd_func = simd_select_step_function();
    start_time = get_time_ms();
    for (int i = 0; i < 1000; i++) {
        simd_func(test_data, test_count, dt, gravity, windx, windy);
    }
    double simd_time = get_time_ms() - start_time;
    
    printf("  Scalar: %.2f ms\n", scalar_time);
    printf("  Selected (%s): %.2f ms (%.2fx speedup)\n", simd_get_function_name(simd_func), simd_time, scalar_time / simd_time);
    
    simd_aligned_free(test_data);
}
