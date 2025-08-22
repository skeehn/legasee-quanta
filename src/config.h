#ifndef CONFIG_H
#define CONFIG_H

#include "error.h"
#include <stdint.h>

/* Configuration structure for all simulation parameters */
typedef struct {
    /* Physics parameters */
    float default_gravity;
    float default_wind_x;
    float default_wind_y;
    float particle_lifetime;
    float collision_damping;
    float friction_coefficient;
    
    /* Performance parameters */
    int default_max_particles;
    int default_target_fps;
    int enable_simd;
    int enable_profiling;
    int enable_debug_output;
    int enable_memory_tracking;
    
    /* Rendering parameters */
    char default_particle_glyph;
    uint32_t default_particle_color;
    int color_by_velocity;
    int color_by_energy;
    int enable_particle_trails;
    int trail_length;
    
    /* System parameters */
    int terminal_min_width;
    int terminal_min_height;
    int enable_signal_handling;
    int enable_cleanup_on_exit;
    
    /* Input parameters */
    float wind_step;
    float gravity_step;
    int burst_count;
    float burst_spread;
    
    /* SIMD parameters */
    int simd_auto_tune;
    int simd_preferred_level;
    int simd_benchmark_on_startup;
    
    /* Memory parameters */
    int pool_initial_size;
    int pool_growth_factor;
    int enable_memory_pooling;
    
    /* Logging parameters */
    int log_level;
    const char *log_file;
    int log_to_console;
    int log_to_file;
    
    /* Advanced parameters */
    float time_step;
    int max_iterations_per_frame;
    float boundary_elasticity;
    int enable_particle_interactions;
} GlobalConfig;

/* Configuration validation structure */
typedef struct {
    int min_particles;
    int max_particles;
    float min_gravity;
    float max_gravity;
    float min_wind;
    float max_wind;
    int min_fps;
    int max_fps;
    int min_terminal_width;
    int max_terminal_width;
    int min_terminal_height;
    int max_terminal_height;
} ConfigValidation;

/* Default configuration values */
extern const GlobalConfig DEFAULT_CONFIG;
extern const ConfigValidation CONFIG_VALIDATION;

/* Configuration management functions */
Error config_init(void);
void config_cleanup(void);

/* Configuration loading and saving */
Error config_load_from_file(const char *filename, GlobalConfig *config);
Error config_save_to_file(const char *filename, const GlobalConfig *config);
Error config_load_from_env(GlobalConfig *config);
Error config_load_defaults(GlobalConfig *config);

/* Configuration validation */
Error config_validate(const GlobalConfig *config);
Error config_validate_field(const GlobalConfig *config, const char *field_name);
Error config_validate_range(const char *field_name, float value, float min, float max);
Error config_validate_range_int(const char *field_name, int value, int min, int max);

/* Configuration access and modification */
const GlobalConfig* config_get_global(void);
Error config_set_global(const GlobalConfig *config);
Error config_get_value(const char *key, void *value, size_t size);
Error config_set_value(const char *key, const void *value, size_t size);

/* Configuration utilities */
Error config_merge(GlobalConfig *target, const GlobalConfig *source);
Error config_diff(const GlobalConfig *config1, const GlobalConfig *config2, char *diff_buffer, size_t buffer_size);
void config_print(const GlobalConfig *config);
void config_print_diff(const GlobalConfig *config1, const GlobalConfig *config2);

/* Configuration presets */
Error config_load_preset(const char *preset_name, GlobalConfig *config);
Error config_save_preset(const char *preset_name, const GlobalConfig *config);
Error config_list_presets(char *buffer, size_t buffer_size);

/* Configuration change callbacks */
typedef void (*ConfigChangeCallback)(const char *key, const void *old_value, const void *new_value, void *user_data);

Error config_register_change_callback(const char *key, ConfigChangeCallback callback, void *user_data);
Error config_unregister_change_callback(const char *key, ConfigChangeCallback callback);

/* Configuration statistics */
typedef struct {
    size_t load_count;
    size_t save_count;
    size_t validation_errors;
    size_t change_callbacks;
    const char *last_loaded_file;
    time_t last_modified;
} ConfigStats;

ConfigStats config_get_stats(void);
void config_reset_stats(void);
void config_print_stats(void);

/* Configuration file format support */
typedef enum {
    CONFIG_FORMAT_INI,
    CONFIG_FORMAT_JSON,
    CONFIG_FORMAT_YAML,
    CONFIG_FORMAT_ENV
} ConfigFormat;

Error config_load_from_file_with_format(const char *filename, ConfigFormat format, GlobalConfig *config);
Error config_save_to_file_with_format(const char *filename, ConfigFormat format, const GlobalConfig *config);

/* Environment variable mapping */
typedef struct {
    const char *env_var;
    const char *config_key;
    const char *description;
} EnvVarMapping;

extern const EnvVarMapping ENV_VAR_MAPPINGS[];
extern const size_t ENV_VAR_MAPPINGS_COUNT;

/* Configuration documentation */
void config_print_help(void);
void config_print_schema(void);
void config_print_example(void);

#endif /* CONFIG_H */
