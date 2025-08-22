#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

/* Global log configuration */
static LogConfig g_log_config = {0};
static int g_log_initialized = 0;
static LogStats g_log_stats = {0};
static FILE *g_log_file = NULL;

/* Log level strings */
static const char* LOG_LEVEL_STRINGS[] = {
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_WARN] = "WARN",
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_FATAL] = "FATAL"
};

/* Log format strings */
static const char* LOG_FORMAT_STRINGS[] = {
    [LOG_FORMAT_SIMPLE] = "SIMPLE",
    [LOG_FORMAT_DETAILED] = "DETAILED",
    [LOG_FORMAT_JSON] = "JSON",
    [LOG_FORMAT_CSV] = "CSV"
};

/* Default configurations */
const LogConfig LOG_CONFIG_DEFAULT = {
    .level = LOG_LEVEL_INFO,
    .output = LOG_OUTPUT_CONSOLE,
    .format = LOG_FORMAT_SIMPLE,
    .file_path = NULL,
    .max_file_size = 1024 * 1024,
    .max_files = 5,
    .enable_timestamps = 1,
    .enable_thread_info = 0,
    .enable_color = 0,
    .enable_performance_tracking = 0,
    .buffer_size = 4096,
    .flush_interval = 1000
};

const LogConfig LOG_CONFIG_DEBUG = {
    .level = LOG_LEVEL_DEBUG,
    .output = LOG_OUTPUT_BOTH,
    .format = LOG_FORMAT_DETAILED,
    .file_path = "debug.log",
    .max_file_size = 1024 * 1024,
    .max_files = 10,
    .enable_timestamps = 1,
    .enable_thread_info = 1,
    .enable_color = 1,
    .enable_performance_tracking = 1,
    .buffer_size = 8192,
    .flush_interval = 100
};

const LogConfig LOG_CONFIG_PRODUCTION = {
    .level = LOG_LEVEL_WARN,
    .output = LOG_OUTPUT_FILE,
    .format = LOG_FORMAT_JSON,
    .file_path = "production.log",
    .max_file_size = 10 * 1024 * 1024,
    .max_files = 5,
    .enable_timestamps = 1,
    .enable_thread_info = 0,
    .enable_color = 0,
    .enable_performance_tracking = 0,
    .buffer_size = 16384,
    .flush_interval = 5000
};

const LogConfig LOG_CONFIG_PERFORMANCE = {
    .level = LOG_LEVEL_ERROR,
    .output = LOG_OUTPUT_CONSOLE,
    .format = LOG_FORMAT_SIMPLE,
    .file_path = NULL,
    .max_file_size = 0,
    .max_files = 0,
    .enable_timestamps = 0,
    .enable_thread_info = 0,
    .enable_color = 0,
    .enable_performance_tracking = 1,
    .buffer_size = 1024,
    .flush_interval = 10000
};

/* Initialize logging system */
Error log_init(const LogConfig *config) {
    if (g_log_initialized) {
        return ERROR_CREATE(ERROR_INVALID_STATE, "Logging system already initialized");
    }
    
    if (config) {
        memcpy(&g_log_config, config, sizeof(LogConfig));
    } else {
        memcpy(&g_log_config, &LOG_CONFIG_DEFAULT, sizeof(LogConfig));
    }
    
    /* Initialize statistics */
    g_log_stats.start_time = time(NULL);
    g_log_stats.last_message_time = g_log_stats.start_time;
    
    /* Open log file if needed */
    if ((g_log_config.output & LOG_OUTPUT_FILE) && g_log_config.file_path) {
        g_log_file = fopen(g_log_config.file_path, "a");
        if (!g_log_file) {
            return ERROR_CREATE(ERROR_FILE_WRITE_FAILED, "Cannot open log file");
        }
    }
    
    g_log_initialized = 1;
    
    /* Log initialization message */
    LOG_INFO("Logging system initialized with level %s", 
             log_level_to_string(g_log_config.level));
    
    return (Error){SUCCESS};
}

/* Cleanup logging system */
void log_cleanup(void) {
    if (!g_log_initialized) return;
    
    LOG_INFO("Logging system shutting down");
    
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    g_log_initialized = 0;
}

/* Get current timestamp string */
static void get_timestamp(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* Format log message */
static void format_message(char *buffer, size_t size, LogLevel level, 
                          const char *file, int line, const char *function, 
                          const char *format, va_list args) {
    char timestamp[32] = {0};
    char message[512] = {0};
    
    /* Get timestamp if enabled */
    if (g_log_config.enable_timestamps) {
        get_timestamp(timestamp, sizeof(timestamp));
    }
    
    /* Format the message */
    vsnprintf(message, sizeof(message), format, args);
    
    /* Format based on configuration */
    switch (g_log_config.format) {
        case LOG_FORMAT_SIMPLE:
            if (g_log_config.enable_timestamps) {
                snprintf(buffer, size, "[%s] %s: %s\n", 
                        timestamp, log_level_to_string(level), message);
            } else {
                snprintf(buffer, size, "%s: %s\n", 
                        log_level_to_string(level), message);
            }
            break;
            
        case LOG_FORMAT_DETAILED:
            if (g_log_config.enable_timestamps) {
                snprintf(buffer, size, "[%s] %s [%s:%d:%s] %s\n", 
                        timestamp, log_level_to_string(level), 
                        file ? file : "unknown", line, 
                        function ? function : "unknown", message);
            } else {
                snprintf(buffer, size, "%s [%s:%d:%s] %s\n", 
                        log_level_to_string(level), 
                        file ? file : "unknown", line, 
                        function ? function : "unknown", message);
            }
            break;
            
        case LOG_FORMAT_JSON:
            snprintf(buffer, size, 
                    "{\"timestamp\":\"%s\",\"level\":\"%s\",\"file\":\"%s\","
                    "\"line\":%d,\"function\":\"%s\",\"message\":\"%s\"}\n",
                    timestamp, log_level_to_string(level), 
                    file ? file : "unknown", line, 
                    function ? function : "unknown", message);
            break;
            
        case LOG_FORMAT_CSV:
            snprintf(buffer, size, "\"%s\",\"%s\",\"%s\",%d,\"%s\",\"%s\"\n",
                    timestamp, log_level_to_string(level), 
                    file ? file : "unknown", line, 
                    function ? function : "unknown", message);
            break;
    }
}

/* Core logging function */
void log_message(LogLevel level, const char *file, int line, const char *function, 
                const char *format, ...) {
    if (!g_log_initialized || level < g_log_config.level) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    log_message_va(level, file, line, function, format, args);
    va_end(args);
}

/* Core logging function with va_list */
void log_message_va(LogLevel level, const char *file, int line, const char *function, 
                   const char *format, va_list args) {
    if (!g_log_initialized || level < g_log_config.level) {
        return;
    }
    
    char buffer[1024];
    format_message(buffer, sizeof(buffer), level, file, line, function, format, args);
    
    size_t message_len = strlen(buffer);
    
    /* Update statistics */
    g_log_stats.messages_by_level[level]++;
    g_log_stats.total_messages++;
    g_log_stats.bytes_written += message_len;
    g_log_stats.last_message_time = time(NULL);
    
    if (message_len > g_log_stats.max_message_length) {
        g_log_stats.max_message_length = message_len;
    }
    
    /* Calculate average message length */
    g_log_stats.average_message_length = 
        g_log_stats.bytes_written / g_log_stats.total_messages;
    
    /* Output to console */
    if (g_log_config.output & LOG_OUTPUT_CONSOLE) {
        fputs(buffer, stderr);
    }
    
    /* Output to file */
    if ((g_log_config.output & LOG_OUTPUT_FILE) && g_log_file) {
        fputs(buffer, g_log_file);
        fflush(g_log_file);
    }
}

/* Performance tracking functions */
void log_performance_start(LogPerformance *perf) {
    if (!perf) return;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    perf->start_time = ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void log_performance_end(LogPerformance *perf) {
    if (!perf) return;
    
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    perf->end_time = ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
    perf->duration = perf->end_time - perf->start_time;
    
    LOG_DEBUG("Performance: %s took %.3f ms", perf->operation, perf->duration);
}

void log_performance(const char *operation, double duration_ms, 
                    const char *file, int line, const char *function) {
    LOG_DEBUG("Performance: %s took %.3f ms", operation, duration_ms);
}

/* Log utility functions */
const char* log_level_to_string(LogLevel level) {
    if (level >= 0 && level < LOG_LEVEL_COUNT) {
        return LOG_LEVEL_STRINGS[level];
    }
    return "UNKNOWN";
}

LogLevel log_string_to_level(const char *level_str) {
    if (!level_str) return LOG_LEVEL_INFO;
    
    for (int i = 0; i < LOG_LEVEL_COUNT; i++) {
        if (strcasecmp(level_str, LOG_LEVEL_STRINGS[i]) == 0) {
            return (LogLevel)i;
        }
    }
    return LOG_LEVEL_INFO;
}

const char* log_format_to_string(LogFormat format) {
    if (format >= 0 && format < 4) {
        return LOG_FORMAT_STRINGS[format];
    }
    return "UNKNOWN";
}

LogFormat log_string_to_format(const char *format_str) {
    if (!format_str) return LOG_FORMAT_SIMPLE;
    
    for (int i = 0; i < 4; i++) {
        if (strcasecmp(format_str, LOG_FORMAT_STRINGS[i]) == 0) {
            return (LogFormat)i;
        }
    }
    return LOG_FORMAT_SIMPLE;
}

/* Log statistics */
LogStats log_get_stats(void) {
    return g_log_stats;
}

void log_reset_stats(void) {
    memset(&g_log_stats, 0, sizeof(LogStats));
    g_log_stats.start_time = time(NULL);
    g_log_stats.last_message_time = g_log_stats.start_time;
}

void log_print_stats(void) {
    printf("=== Log Statistics ===\n");
    printf("Total Messages: %zu\n", g_log_stats.total_messages);
    printf("Bytes Written: %zu\n", g_log_stats.bytes_written);
    printf("Max Message Length: %zu\n", g_log_stats.max_message_length);
    printf("Average Message Length: %zu\n", g_log_stats.average_message_length);
    
    printf("Messages by Level:\n");
    for (int i = 0; i < LOG_LEVEL_COUNT; i++) {
        if (g_log_stats.messages_by_level[i] > 0) {
            printf("  %s: %zu\n", log_level_to_string(i), g_log_stats.messages_by_level[i]);
        }
    }
    printf("===================\n");
}

void log_print_config(void) {
    printf("=== Log Configuration ===\n");
    printf("Level: %s\n", log_level_to_string(g_log_config.level));
    printf("Output: %d\n", g_log_config.output);
    printf("Format: %s\n", log_format_to_string(g_log_config.format));
    printf("File: %s\n", g_log_config.file_path ? g_log_config.file_path : "none");
    printf("Timestamps: %s\n", g_log_config.enable_timestamps ? "enabled" : "disabled");
    printf("Color: %s\n", g_log_config.enable_color ? "enabled" : "disabled");
    printf("Performance Tracking: %s\n", g_log_config.enable_performance_tracking ? "enabled" : "disabled");
    printf("========================\n");
}

/* Log configuration functions */
Error log_set_level(LogLevel level) {
    if (level < 0 || level >= LOG_LEVEL_COUNT) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Invalid log level");
    }
    g_log_config.level = level;
    return (Error){SUCCESS};
}

Error log_set_output(LogOutput output) {
    g_log_config.output = output;
    return (Error){SUCCESS};
}

Error log_set_format(LogFormat format) {
    if (format < 0 || format >= 4) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Invalid log format");
    }
    g_log_config.format = format;
    return (Error){SUCCESS};
}

Error log_set_file(const char *file_path) {
    /* Close existing file */
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = NULL;
    }
    
    /* Open new file */
    if (file_path) {
        g_log_file = fopen(file_path, "a");
        if (!g_log_file) {
            return ERROR_CREATE(ERROR_FILE_WRITE_FAILED, "Cannot open log file");
        }
    }
    
    g_log_config.file_path = file_path;
    return (Error){SUCCESS};
}

Error log_set_config(const LogConfig *config) {
    if (!config) {
        return ERROR_CREATE(ERROR_NULL_POINTER, "Config is NULL");
    }
    
    memcpy(&g_log_config, config, sizeof(LogConfig));
    return (Error){SUCCESS};
}

/* Log callbacks */
Error log_register_callback(LogCallback callback, void *user_data) {
    /* Simplified implementation - in a full system, you'd maintain a callback list */
    return (Error){SUCCESS};
}

Error log_unregister_callback(LogCallback callback) {
    /* Simplified implementation */
    return (Error){SUCCESS};
}

/* Error integration */
void log_error(const Error *error) {
    if (!error) return;
    
    LOG_ERROR("Error %s: %s", 
              error_code_to_string(error->code),
              error_get_description(error));
}

void log_error_with_context(const Error *error, const char *context) {
    if (!error) return;
    
    LOG_ERROR("Error %s in %s: %s", 
              error_code_to_string(error->code),
              context ? context : "unknown context",
              error_get_description(error));
}

/* Log initialization helpers */
Error log_init_default(void) {
    return log_init(&LOG_CONFIG_DEFAULT);
}

Error log_init_debug(void) {
    return log_init(&LOG_CONFIG_DEBUG);
}

Error log_init_production(void) {
    return log_init(&LOG_CONFIG_PRODUCTION);
}

Error log_init_performance(void) {
    return log_init(&LOG_CONFIG_PERFORMANCE);
}

/* Stub implementations for unimplemented functions */
void log_flush(void) {
    if (g_log_file) {
        fflush(g_log_file);
    }
}

void log_sync(void) {
    if (g_log_file) {
        fflush(g_log_file);
        fsync(fileno(g_log_file));
    }
}

Error log_rotate_file(void) {
    /* Simplified implementation */
    return (Error){SUCCESS};
}

Error log_clear_file(void) {
    if (g_log_file) {
        fclose(g_log_file);
        g_log_file = fopen(g_log_config.file_path, "w");
        if (!g_log_file) {
            return ERROR_CREATE(ERROR_FILE_WRITE_FAILED, "Cannot clear log file");
        }
    }
    return (Error){SUCCESS};
}

Error log_set_max_file_size(int max_size) {
    g_log_config.max_file_size = max_size;
    return (Error){SUCCESS};
}

Error log_set_max_files(int max_files) {
    g_log_config.max_files = max_files;
    return (Error){SUCCESS};
}

Error log_set_filter(const char *filter_pattern) {
    /* Simplified implementation */
    return (Error){SUCCESS};
}

Error log_set_custom_formatter(const char *format_string) {
    /* Simplified implementation */
    return (Error){SUCCESS};
}

void log_set_timestamp_format(const char *format) {
    /* Simplified implementation */
}

Error log_set_buffer_size(int buffer_size) {
    g_log_config.buffer_size = buffer_size;
    return (Error){SUCCESS};
}

Error log_set_flush_interval(int interval_ms) {
    g_log_config.flush_interval = interval_ms;
    return (Error){SUCCESS};
}

Error log_set_thread_safe(int enabled) {
    /* Simplified implementation */
    return (Error){SUCCESS};
}

int log_is_thread_safe(void) {
    return 0; /* Simplified implementation */
}

/* Memory tracking integration */
void log_memory_allocated(size_t size, const char *file, int line) {
    LOG_DEBUG("Memory allocated: %zu bytes at %s:%d", size, file, line);
}

void log_memory_freed(size_t size, const char *file, int line) {
    LOG_DEBUG("Memory freed: %zu bytes at %s:%d", size, file, line);
}

void log_memory_peak(size_t peak_size) {
    LOG_INFO("Memory peak: %zu bytes", peak_size);
}

/* System information logging */
void log_system_info(void) {
    LOG_INFO("System information logged");
}

void log_platform_info(void) {
    LOG_INFO("Platform information logged");
}

void log_compiler_info(void) {
    LOG_INFO("Compiler information logged");
}

/* Log buffer functions */
LogBuffer* log_buffer_create(size_t initial_size) {
    LogBuffer *buffer = malloc(sizeof(LogBuffer));
    if (!buffer) return NULL;
    
    buffer->buffer = malloc(initial_size);
    if (!buffer->buffer) {
        free(buffer);
        return NULL;
    }
    
    buffer->size = initial_size;
    buffer->used = 0;
    buffer->buffer[0] = '\0';
    
    return buffer;
}

void log_buffer_destroy(LogBuffer *buffer) {
    if (buffer) {
        free(buffer->buffer);
        free(buffer);
    }
}

Error log_buffer_append(LogBuffer *buffer, const char *format, ...) {
    if (!buffer) return ERROR_CREATE(ERROR_NULL_POINTER, "Buffer is NULL");
    
    va_list args;
    va_start(args, format);
    Error result = log_buffer_append_va(buffer, format, args);
    va_end(args);
    
    return result;
}

Error log_buffer_append_va(LogBuffer *buffer, const char *format, va_list args) {
    if (!buffer) return ERROR_CREATE(ERROR_NULL_POINTER, "Buffer is NULL");
    
    /* Calculate required size */
    va_list args_copy;
    va_copy(args_copy, args);
    int needed = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    
    if (needed < 0) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Invalid format string");
    }
    
    /* Ensure buffer has enough space */
    if (buffer->used + needed + 1 > buffer->size) {
        size_t new_size = buffer->size * 2;
        if (new_size < buffer->used + needed + 1) {
            new_size = buffer->used + needed + 1;
        }
        
        char *new_buffer = realloc(buffer->buffer, new_size);
        if (!new_buffer) {
            return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Cannot resize buffer");
        }
        
        buffer->buffer = new_buffer;
        buffer->size = new_size;
    }
    
    /* Append the formatted string */
    vsnprintf(buffer->buffer + buffer->used, buffer->size - buffer->used, format, args);
    buffer->used += needed;
    
    return (Error){SUCCESS};
}

void log_buffer_clear(LogBuffer *buffer) {
    if (buffer) {
        buffer->used = 0;
        buffer->buffer[0] = '\0';
    }
}

const char* log_buffer_get_string(const LogBuffer *buffer) {
    return buffer ? buffer->buffer : NULL;
}
