#ifndef LOG_H
#define LOG_H

#include "error.h"
#include <stdarg.h>
#include <time.h>

/* Log levels */
typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL,
    LOG_LEVEL_COUNT
} LogLevel;

/* Log output destinations */
typedef enum {
    LOG_OUTPUT_NONE = 0,
    LOG_OUTPUT_CONSOLE = 1,
    LOG_OUTPUT_FILE = 2,
    LOG_OUTPUT_BOTH = 3
} LogOutput;

/* Log format options */
typedef enum {
    LOG_FORMAT_SIMPLE,
    LOG_FORMAT_DETAILED,
    LOG_FORMAT_JSON,
    LOG_FORMAT_CSV
} LogFormat;

/* Log statistics */
typedef struct {
    size_t messages_by_level[LOG_LEVEL_COUNT];
    size_t total_messages;
    size_t bytes_written;
    time_t start_time;
    time_t last_message_time;
    size_t max_message_length;
    size_t average_message_length;
} LogStats;

/* Log configuration */
typedef struct {
    LogLevel level;
    LogOutput output;
    LogFormat format;
    const char *file_path;
    int max_file_size;
    int max_files;
    int enable_timestamps;
    int enable_thread_info;
    int enable_color;
    int enable_performance_tracking;
    int buffer_size;
    int flush_interval;
} LogConfig;

/* Performance tracking structure */
typedef struct {
    const char *operation;
    double start_time;
    double end_time;
    double duration;
    const char *file;
    int line;
    const char *function;
} LogPerformance;

/* Log initialization and cleanup */
Error log_init(const LogConfig *config);
void log_cleanup(void);

/* Core logging functions */
void log_message(LogLevel level, const char *file, int line, const char *function, const char *format, ...);
void log_message_va(LogLevel level, const char *file, int line, const char *function, const char *format, va_list args);

/* Convenience logging macros */
#define LOG_DEBUG(format, ...) \
    log_message(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_INFO(format, ...) \
    log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_WARN(format, ...) \
    log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_ERROR(format, ...) \
    log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#define LOG_FATAL(format, ...) \
    log_message(LOG_LEVEL_FATAL, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

/* Performance logging macros */
#define LOG_PERF_START(operation) \
    LogPerformance _perf = {operation, 0, 0, 0, __FILE__, __LINE__, __func__}; \
    log_performance_start(&_perf)

#define LOG_PERF_END() \
    log_performance_end(&_perf)

#define LOG_PERF(operation, duration_ms) \
    log_performance(operation, duration_ms, __FILE__, __LINE__, __func__)

/* Performance tracking functions */
void log_performance_start(LogPerformance *perf);
void log_performance_end(LogPerformance *perf);
void log_performance(const char *operation, double duration_ms, const char *file, int line, const char *function);

/* Log configuration functions */
Error log_set_level(LogLevel level);
Error log_set_output(LogOutput output);
Error log_set_format(LogFormat format);
Error log_set_file(const char *file_path);
Error log_set_config(const LogConfig *config);

/* Log utility functions */
const char* log_level_to_string(LogLevel level);
LogLevel log_string_to_level(const char *level_str);
const char* log_format_to_string(LogFormat format);
LogFormat log_string_to_format(const char *format_str);

/* Log statistics and monitoring */
LogStats log_get_stats(void);
void log_reset_stats(void);
void log_print_stats(void);
void log_print_config(void);

/* Log file management */
Error log_rotate_file(void);
Error log_clear_file(void);
Error log_set_max_file_size(int max_size);
Error log_set_max_files(int max_files);

/* Log filtering and formatting */
Error log_set_filter(const char *filter_pattern);
Error log_set_custom_formatter(const char *format_string);
void log_set_timestamp_format(const char *format);

/* Log buffering and flushing */
Error log_set_buffer_size(int buffer_size);
Error log_set_flush_interval(int interval_ms);
void log_flush(void);
void log_sync(void);

/* Log callbacks */
typedef void (*LogCallback)(LogLevel level, const char *message, void *user_data);

Error log_register_callback(LogCallback callback, void *user_data);
Error log_unregister_callback(LogCallback callback);

/* Default configurations */
extern const LogConfig LOG_CONFIG_DEFAULT;
extern const LogConfig LOG_CONFIG_DEBUG;
extern const LogConfig LOG_CONFIG_PRODUCTION;
extern const LogConfig LOG_CONFIG_PERFORMANCE;

/* Log initialization helpers */
Error log_init_default(void);
Error log_init_debug(void);
Error log_init_production(void);
Error log_init_performance(void);

/* Log message formatting */
typedef struct {
    char *buffer;
    size_t size;
    size_t used;
} LogBuffer;

LogBuffer* log_buffer_create(size_t initial_size);
void log_buffer_destroy(LogBuffer *buffer);
Error log_buffer_append(LogBuffer *buffer, const char *format, ...);
Error log_buffer_append_va(LogBuffer *buffer, const char *format, va_list args);
void log_buffer_clear(LogBuffer *buffer);
const char* log_buffer_get_string(const LogBuffer *buffer);

/* Thread safety */
Error log_set_thread_safe(int enabled);
int log_is_thread_safe(void);

/* Error integration */
void log_error(const Error *error);
void log_error_with_context(const Error *error, const char *context);

/* Memory tracking integration */
void log_memory_allocated(size_t size, const char *file, int line);
void log_memory_freed(size_t size, const char *file, int line);
void log_memory_peak(size_t peak_size);

/* System information logging */
void log_system_info(void);
void log_platform_info(void);
void log_compiler_info(void);

#endif /* LOG_H */
