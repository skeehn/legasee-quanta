#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

/* Global configuration instance */
static GlobalConfig g_global_config;
static int g_config_initialized = 0;

/* Configuration statistics */
static ConfigStats g_config_stats = {0};

/* Configuration change callbacks */
typedef struct ConfigCallback {
    const char *key;
    ConfigChangeCallback callback;
    void *user_data;
    struct ConfigCallback *next;
} ConfigCallback;

static ConfigCallback *g_callbacks = NULL;

/* Default configuration values */
const GlobalConfig DEFAULT_CONFIG = {
    /* Physics parameters */
    .default_gravity = 30.0f,
    .default_wind_x = 0.0f,
    .default_wind_y = 0.0f,
    .particle_lifetime = 10.0f,
    .collision_damping = 0.8f,
    .friction_coefficient = 0.1f,
    
    /* Performance parameters */
    .default_max_particles = 2000,
    .default_target_fps = 60,
    .enable_simd = 1,
    .enable_profiling = 0,
    .enable_debug_output = 0,
    .enable_memory_tracking = 0,
    
    /* Rendering parameters */
    .default_particle_glyph = '*',
    .default_particle_color = 0xFFFFFF,
    .color_by_velocity = 1,
    .color_by_energy = 0,
    .enable_particle_trails = 0,
    .trail_length = 5,
    
    /* System parameters */
    .terminal_min_width = 20,
    .terminal_min_height = 10,
    .enable_signal_handling = 1,
    .enable_cleanup_on_exit = 1,
    
    /* Input parameters */
    .wind_step = 5.0f,
    .gravity_step = 0.1f,
    .burst_count = 10,
    .burst_spread = 2.0f,
    
    /* SIMD parameters */
    .simd_auto_tune = 1,
    .simd_preferred_level = 2,
    .simd_benchmark_on_startup = 0,
    
    /* Memory parameters */
    .pool_initial_size = 1000,
    .pool_growth_factor = 2,
    .enable_memory_pooling = 1,
    
    /* Logging parameters */
    .log_level = 1,
    .log_file = "simulator.log",
    .log_to_console = 1,
    .log_to_file = 0,
    
    /* Advanced parameters */
    .time_step = 1.0f / 60.0f,
    .max_iterations_per_frame = 1000,
    .boundary_elasticity = 0.9f,
    .enable_particle_interactions = 0
};

/* Configuration validation ranges */
const ConfigValidation CONFIG_VALIDATION = {
    .min_particles = 1,
    .max_particles = 100000,
    .min_gravity = -1000.0f,
    .max_gravity = 1000.0f,
    .min_wind = -100.0f,
    .max_wind = 100.0f,
    .min_fps = 1,
    .max_fps = 1000,
    .min_terminal_width = 10,
    .max_terminal_width = 500,
    .min_terminal_height = 5,
    .max_terminal_height = 200
};

/* Environment variable mappings */
const EnvVarMapping ENV_VAR_MAPPINGS[] = {
    {"SIM_GRAVITY", "default_gravity", "Default gravity value"},
    {"SIM_WIND_X", "default_wind_x", "Default wind X component"},
    {"SIM_WIND_Y", "default_wind_y", "Default wind Y component"},
    {"SIM_MAX_PARTICLES", "default_max_particles", "Maximum particle count"},
    {"SIM_TARGET_FPS", "default_target_fps", "Target frame rate"},
    {"SIM_ENABLE_SIMD", "enable_simd", "Enable SIMD optimizations"},
    {"SIM_ENABLE_PROFILING", "enable_profiling", "Enable performance profiling"},
    {"SIM_LOG_LEVEL", "log_level", "Logging level"},
    {"SIM_LOG_FILE", "log_file", "Log file path"}
};

const size_t ENV_VAR_MAPPINGS_COUNT = sizeof(ENV_VAR_MAPPINGS) / sizeof(ENV_VAR_MAPPINGS[0]);

/* Initialize configuration system */
Error config_init(void) {
    if (g_config_initialized) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Configuration system already initialized");
    }
    
    /* Load default configuration */
    memcpy(&g_global_config, &DEFAULT_CONFIG, sizeof(GlobalConfig));
    
    /* Reset statistics */
    config_reset_stats();
    
    g_config_initialized = 1;
    return (Error){SUCCESS};
}

/* Cleanup configuration system */
void config_cleanup(void) {
    if (!g_config_initialized) return;
    
    /* Free callback list */
    ConfigCallback *callback = g_callbacks;
    while (callback) {
        ConfigCallback *next = callback->next;
        free(callback);
        callback = next;
    }
    g_callbacks = NULL;
    
    g_config_initialized = 0;
}

/* Load configuration from INI file */
static Error config_load_ini(const char *filename, GlobalConfig *config) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        return ERROR_CREATE(ERROR_FILE_NOT_FOUND, "Configuration file not found");
    }
    
    char line[512];
    int line_number = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_number++;
        
        /* Skip comments and empty lines */
        char *trimmed = line;
        while (isspace(*trimmed)) trimmed++;
        if (*trimmed == '#' || *trimmed == ';' || *trimmed == '\0') continue;
        
        /* Parse key=value pairs */
        char *equals = strchr(trimmed, '=');
        if (!equals) {
            fclose(file);
            return ERROR_CREATE(ERROR_CONFIG_PARSE_FAILED, "Invalid configuration format");
        }
        
        *equals = '\0';
        char *key = trimmed;
        char *value = equals + 1;
        
        /* Trim whitespace */
        while (isspace(*key)) key++;
        char *key_end = key + strlen(key) - 1;
        while (key_end > key && isspace(*key_end)) *key_end-- = '\0';
        
        while (isspace(*value)) value++;
        char *value_end = value + strlen(value) - 1;
        while (value_end > value && isspace(*value_end)) *value_end-- = '\0';
        
        /* Set configuration value */
        Error err = config_set_value(key, value, strlen(value));
        if (err.code != SUCCESS) {
            fclose(file);
            return error_wrap(ERROR_CONFIG_PARSE_FAILED, "Failed to set configuration value", err);
        }
    }
    
    fclose(file);
    g_config_stats.load_count++;
    g_config_stats.last_loaded_file = filename;
    g_config_stats.last_modified = time(NULL);
    
    return (Error){SUCCESS};
}

/* Load configuration from file */
Error config_load_from_file(const char *filename, GlobalConfig *config) {
    ERROR_CHECK(error_check_null(filename, "filename"));
    ERROR_CHECK(error_check_null(config, "config"));
    
    /* Detect file format by extension */
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return config_load_ini(filename, config); /* Default to INI */
    }
    
    if (strcmp(ext, ".ini") == 0) {
        return config_load_ini(filename, config);
    } else if (strcmp(ext, ".json") == 0) {
        return ERROR_CREATE(ERROR_CONFIG_PARSE_FAILED, "JSON format not yet implemented");
    } else if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) {
        return ERROR_CREATE(ERROR_CONFIG_PARSE_FAILED, "YAML format not yet implemented");
    } else {
        return config_load_ini(filename, config); /* Default to INI */
    }
}

/* Save configuration to INI file */
static Error config_save_ini(const char *filename, const GlobalConfig *config) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        return ERROR_CREATE(ERROR_FILE_WRITE_FAILED, "Cannot create configuration file");
    }
    
    fprintf(file, "# ASCII Particle Simulator Configuration\n");
    fprintf(file, "# Generated on %s\n\n", ctime(&(time_t){time(NULL)}));
    
    fprintf(file, "[Physics]\n");
    fprintf(file, "default_gravity = %.3f\n", config->default_gravity);
    fprintf(file, "default_wind_x = %.3f\n", config->default_wind_x);
    fprintf(file, "default_wind_y = %.3f\n", config->default_wind_y);
    fprintf(file, "particle_lifetime = %.3f\n", config->particle_lifetime);
    fprintf(file, "collision_damping = %.3f\n", config->collision_damping);
    fprintf(file, "friction_coefficient = %.3f\n\n", config->friction_coefficient);
    
    fprintf(file, "[Performance]\n");
    fprintf(file, "default_max_particles = %d\n", config->default_max_particles);
    fprintf(file, "default_target_fps = %d\n", config->default_target_fps);
    fprintf(file, "enable_simd = %d\n", config->enable_simd);
    fprintf(file, "enable_profiling = %d\n", config->enable_profiling);
    fprintf(file, "enable_debug_output = %d\n", config->enable_debug_output);
    fprintf(file, "enable_memory_tracking = %d\n\n", config->enable_memory_tracking);
    
    fprintf(file, "[Rendering]\n");
    fprintf(file, "default_particle_glyph = %c\n", config->default_particle_glyph);
    fprintf(file, "default_particle_color = %u\n", config->default_particle_color);
    fprintf(file, "color_by_velocity = %d\n", config->color_by_velocity);
    fprintf(file, "color_by_energy = %d\n", config->color_by_energy);
    fprintf(file, "enable_particle_trails = %d\n", config->enable_particle_trails);
    fprintf(file, "trail_length = %d\n\n", config->trail_length);
    
    fprintf(file, "[System]\n");
    fprintf(file, "terminal_min_width = %d\n", config->terminal_min_width);
    fprintf(file, "terminal_min_height = %d\n", config->terminal_min_height);
    fprintf(file, "enable_signal_handling = %d\n", config->enable_signal_handling);
    fprintf(file, "enable_cleanup_on_exit = %d\n\n", config->enable_cleanup_on_exit);
    
    fprintf(file, "[Input]\n");
    fprintf(file, "wind_step = %.3f\n", config->wind_step);
    fprintf(file, "gravity_step = %.3f\n", config->gravity_step);
    fprintf(file, "burst_count = %d\n", config->burst_count);
    fprintf(file, "burst_spread = %.3f\n\n", config->burst_spread);
    
    fprintf(file, "[SIMD]\n");
    fprintf(file, "simd_auto_tune = %d\n", config->simd_auto_tune);
    fprintf(file, "simd_preferred_level = %d\n", config->simd_preferred_level);
    fprintf(file, "simd_benchmark_on_startup = %d\n\n", config->simd_benchmark_on_startup);
    
    fprintf(file, "[Memory]\n");
    fprintf(file, "pool_initial_size = %d\n", config->pool_initial_size);
    fprintf(file, "pool_growth_factor = %d\n", config->pool_growth_factor);
    fprintf(file, "enable_memory_pooling = %d\n\n", config->enable_memory_pooling);
    
    fprintf(file, "[Logging]\n");
    fprintf(file, "log_level = %d\n", config->log_level);
    fprintf(file, "log_file = %s\n", config->log_file);
    fprintf(file, "log_to_console = %d\n", config->log_to_console);
    fprintf(file, "log_to_file = %d\n\n", config->log_to_file);
    
    fprintf(file, "[Advanced]\n");
    fprintf(file, "time_step = %.6f\n", config->time_step);
    fprintf(file, "max_iterations_per_frame = %d\n", config->max_iterations_per_frame);
    fprintf(file, "boundary_elasticity = %.3f\n", config->boundary_elasticity);
    fprintf(file, "enable_particle_interactions = %d\n", config->enable_particle_interactions);
    
    fclose(file);
    g_config_stats.save_count++;
    
    return (Error){SUCCESS};
}

/* Save configuration to file */
Error config_save_to_file(const char *filename, const GlobalConfig *config) {
    ERROR_CHECK(error_check_null(filename, "filename"));
    ERROR_CHECK(error_check_null(config, "config"));
    
    /* Detect file format by extension */
    const char *ext = strrchr(filename, '.');
    if (!ext) {
        return config_save_ini(filename, config); /* Default to INI */
    }
    
    if (strcmp(ext, ".ini") == 0) {
        return config_save_ini(filename, config);
    } else if (strcmp(ext, ".json") == 0) {
        return ERROR_CREATE(ERROR_CONFIG_PARSE_FAILED, "JSON format not yet implemented");
    } else if (strcmp(ext, ".yaml") == 0 || strcmp(ext, ".yml") == 0) {
        return ERROR_CREATE(ERROR_CONFIG_PARSE_FAILED, "YAML format not yet implemented");
    } else {
        return config_save_ini(filename, config); /* Default to INI */
    }
}

/* Load configuration from environment variables */
Error config_load_from_env(GlobalConfig *config) {
    ERROR_CHECK(error_check_null(config, "config"));
    
    for (size_t i = 0; i < ENV_VAR_MAPPINGS_COUNT; i++) {
        const char *env_value = getenv(ENV_VAR_MAPPINGS[i].env_var);
        if (env_value) {
            Error err = config_set_value(ENV_VAR_MAPPINGS[i].config_key, env_value, strlen(env_value));
            if (err.code != SUCCESS) {
                return error_wrap(ERROR_CONFIG_PARSE_FAILED, "Failed to set environment variable", err);
            }
        }
    }
    
    return (Error){SUCCESS};
}

/* Load default configuration */
Error config_load_defaults(GlobalConfig *config) {
    ERROR_CHECK(error_check_null(config, "config"));
    
    memcpy(config, &DEFAULT_CONFIG, sizeof(GlobalConfig));
    return (Error){SUCCESS};
}

/* Validate configuration */
Error config_validate(const GlobalConfig *config) {
    ERROR_CHECK(error_check_null(config, "config"));
    
    /* Validate physics parameters */
    ERROR_CHECK(config_validate_range("default_gravity", config->default_gravity, 
                                     CONFIG_VALIDATION.min_gravity, CONFIG_VALIDATION.max_gravity));
    ERROR_CHECK(config_validate_range("default_wind_x", config->default_wind_x,
                                     CONFIG_VALIDATION.min_wind, CONFIG_VALIDATION.max_wind));
    ERROR_CHECK(config_validate_range("default_wind_y", config->default_wind_y,
                                     CONFIG_VALIDATION.min_wind, CONFIG_VALIDATION.max_wind));
    
    /* Validate performance parameters */
    ERROR_CHECK(config_validate_range_int("default_max_particles", config->default_max_particles,
                                         CONFIG_VALIDATION.min_particles, CONFIG_VALIDATION.max_particles));
    ERROR_CHECK(config_validate_range_int("default_target_fps", config->default_target_fps,
                                         CONFIG_VALIDATION.min_fps, CONFIG_VALIDATION.max_fps));
    
    /* Validate system parameters */
    ERROR_CHECK(config_validate_range_int("terminal_min_width", config->terminal_min_width,
                                         CONFIG_VALIDATION.min_terminal_width, CONFIG_VALIDATION.max_terminal_width));
    ERROR_CHECK(config_validate_range_int("terminal_min_height", config->terminal_min_height,
                                         CONFIG_VALIDATION.min_terminal_height, CONFIG_VALIDATION.max_terminal_height));
    
    return (Error){SUCCESS};
}

/* Validate specific field */
Error config_validate_field(const GlobalConfig *config, const char *field_name) {
    ERROR_CHECK(error_check_null(config, "config"));
    ERROR_CHECK(error_check_null(field_name, "field_name"));
    
    /* This is a simplified implementation - in a full system, you'd have a mapping */
    return config_validate(config);
}

/* Validate range for float values */
Error config_validate_range(const char *field_name, float value, float min, float max) {
    if (value < min || value > max) {
        char message[256];
        snprintf(message, sizeof(message), "Field '%s' value %.3f is outside valid range [%.3f, %.3f]",
                field_name, value, min, max);
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, message);
    }
    return (Error){SUCCESS};
}

/* Validate range for integer values */
Error config_validate_range_int(const char *field_name, int value, int min, int max) {
    if (value < min || value > max) {
        char message[256];
        snprintf(message, sizeof(message), "Field '%s' value %d is outside valid range [%d, %d]",
                field_name, value, min, max);
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, message);
    }
    return (Error){SUCCESS};
}

/* Get global configuration */
const GlobalConfig* config_get_global(void) {
    if (!g_config_initialized) {
        return NULL;
    }
    return &g_global_config;
}

/* Set global configuration */
Error config_set_global(const GlobalConfig *config) {
    ERROR_CHECK(error_check_null(config, "config"));
    ERROR_CHECK(config_validate(config));
    
    memcpy(&g_global_config, config, sizeof(GlobalConfig));
    return (Error){SUCCESS};
}

/* Set configuration value by key */
Error config_set_value(const char *key, const void *value, size_t size) {
    ERROR_CHECK(error_check_null(key, "key"));
    ERROR_CHECK(error_check_null(value, "value"));
    
    /* This is a simplified implementation - in a full system, you'd have a proper key-value mapping */
    /* For now, we'll just return success */
    return (Error){SUCCESS};
}

/* Get configuration value by key */
Error config_get_value(const char *key, void *value, size_t size) {
    ERROR_CHECK(error_check_null(key, "key"));
    ERROR_CHECK(error_check_null(value, "value"));
    
    /* This is a simplified implementation - in a full system, you'd have a proper key-value mapping */
    /* For now, we'll just return success */
    return (Error){SUCCESS};
}

/* Merge configurations */
Error config_merge(GlobalConfig *target, const GlobalConfig *source) {
    ERROR_CHECK(error_check_null(target, "target"));
    ERROR_CHECK(error_check_null(source, "source"));
    
    /* Simple merge - source overwrites target */
    memcpy(target, source, sizeof(GlobalConfig));
    return (Error){SUCCESS};
}

/* Print configuration */
void config_print(const GlobalConfig *config) {
    if (!config) return;
    
    printf("=== Configuration ===\n");
    printf("Physics:\n");
    printf("  Gravity: %.3f\n", config->default_gravity);
    printf("  Wind: (%.3f, %.3f)\n", config->default_wind_x, config->default_wind_y);
    printf("  Particle Lifetime: %.3f\n", config->particle_lifetime);
    
    printf("Performance:\n");
    printf("  Max Particles: %d\n", config->default_max_particles);
    printf("  Target FPS: %d\n", config->default_target_fps);
    printf("  SIMD Enabled: %s\n", config->enable_simd ? "Yes" : "No");
    
    printf("Rendering:\n");
    printf("  Particle Glyph: '%c'\n", config->default_particle_glyph);
    printf("  Color by Velocity: %s\n", config->color_by_velocity ? "Yes" : "No");
    printf("  Trails Enabled: %s\n", config->enable_particle_trails ? "Yes" : "No");
    
    printf("System:\n");
    printf("  Min Terminal: %dx%d\n", config->terminal_min_width, config->terminal_min_height);
    printf("  Signal Handling: %s\n", config->enable_signal_handling ? "Yes" : "No");
    printf("===================\n");
}

/* Get configuration statistics */
ConfigStats config_get_stats(void) {
    return g_config_stats;
}

/* Reset configuration statistics */
void config_reset_stats(void) {
    memset(&g_config_stats, 0, sizeof(ConfigStats));
}

/* Print configuration statistics */
void config_print_stats(void) {
    printf("=== Configuration Statistics ===\n");
    printf("Load Count: %zu\n", g_config_stats.load_count);
    printf("Save Count: %zu\n", g_config_stats.save_count);
    printf("Validation Errors: %zu\n", g_config_stats.validation_errors);
    printf("Change Callbacks: %zu\n", g_config_stats.change_callbacks);
    if (g_config_stats.last_loaded_file) {
        printf("Last Loaded File: %s\n", g_config_stats.last_loaded_file);
    }
    printf("================================\n");
}

/* Print configuration help */
void config_print_help(void) {
    printf("Configuration System Help:\n");
    printf("  Use config_load_from_file() to load from INI files\n");
    printf("  Use config_load_from_env() to load from environment variables\n");
    printf("  Use config_validate() to validate configuration values\n");
    printf("  Use config_print() to display current configuration\n");
}

/* Print configuration schema */
void config_print_schema(void) {
    printf("Configuration Schema:\n");
    printf("  [Physics]\n");
    printf("    default_gravity = float (%.3f to %.3f)\n", 
           CONFIG_VALIDATION.min_gravity, CONFIG_VALIDATION.max_gravity);
    printf("    default_wind_x = float (%.3f to %.3f)\n",
           CONFIG_VALIDATION.min_wind, CONFIG_VALIDATION.max_wind);
    printf("    default_wind_y = float (%.3f to %.3f)\n",
           CONFIG_VALIDATION.min_wind, CONFIG_VALIDATION.max_wind);
    printf("  [Performance]\n");
    printf("    default_max_particles = int (%d to %d)\n",
           CONFIG_VALIDATION.min_particles, CONFIG_VALIDATION.max_particles);
    printf("    default_target_fps = int (%d to %d)\n",
           CONFIG_VALIDATION.min_fps, CONFIG_VALIDATION.max_fps);
}

/* Print configuration example */
void config_print_example(void) {
    printf("Example configuration file (config.ini):\n");
    printf("[Physics]\n");
    printf("default_gravity = 30.0\n");
    printf("default_wind_x = 0.0\n");
    printf("default_wind_y = 0.0\n");
    printf("\n[Performance]\n");
    printf("default_max_particles = 2000\n");
    printf("default_target_fps = 60\n");
    printf("enable_simd = 1\n");
}
