#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "error.h"

/* Forward declarations */
typedef struct DataSource DataSource;
typedef struct DataRecord DataRecord;
typedef struct DataSchema DataSchema;

/* Data types supported */
typedef enum {
    DATA_TYPE_FLOAT,
    DATA_TYPE_INT,
    DATA_TYPE_STRING,
    DATA_TYPE_TIMESTAMP
} DataType;

/* Column definition */
typedef struct {
    char *name;
    DataType type;
    int index;
} DataColumn;

/* Schema definition - describes the structure of data */
struct DataSchema {
    DataColumn *columns;
    int num_columns;
};

/* Single data record (row) */
struct DataRecord {
    float *float_values;    /* Array of float values */
    int *int_values;        /* Array of int values */
    char **string_values;   /* Array of string values */
    int num_values;
    bool valid;
};

/* Data source capabilities */
typedef enum {
    CAP_SEEKABLE    = 1 << 0,  /* Can seek to specific record */
    CAP_STREAMING   = 1 << 1,  /* Continuous data stream */
    CAP_RANDOM      = 1 << 2,  /* Supports random access */
    CAP_BUFFERED    = 1 << 3   /* Has internal buffering */
} DataCapability;

/* Data source interface - all data sources must implement these */
typedef struct DataSourceInterface {
    /* Initialize data source */
    Error (*init)(DataSource *source, const char *config);

    /* Open data source */
    Error (*open)(DataSource *source);

    /* Close data source */
    void (*close)(DataSource *source);

    /* Get schema information */
    Error (*get_schema)(DataSource *source, DataSchema **schema);

    /* Read next record */
    Error (*read_next)(DataSource *source, DataRecord **record);

    /* Check if more data available */
    bool (*has_next)(DataSource *source);

    /* Reset to beginning (if seekable) */
    Error (*reset)(DataSource *source);

    /* Get capabilities */
    uint32_t (*get_capabilities)(DataSource *source);

    /* Cleanup */
    void (*destroy)(DataSource *source);
} DataSourceInterface;

/* Data source base structure */
struct DataSource {
    const char *name;
    const char *type;
    DataSourceInterface *interface;
    void *private_data;  /* Implementation-specific data */
    DataSchema *schema;
    uint32_t capabilities;
    bool is_open;
};

/* Plugin registration */
typedef struct {
    const char *type_name;
    DataSource* (*create)(void);
} DataSourcePlugin;

/* Plugin registry functions */
Error datasource_register_plugin(const char *type, DataSource* (*create_fn)(void));
DataSource* datasource_create(const char *type);
void datasource_list_plugins(void);

/* Helper functions */
Error datasource_init(DataSource *source, const char *config);
Error datasource_open(DataSource *source);
void datasource_close(DataSource *source);
Error datasource_get_schema(DataSource *source, DataSchema **schema);
Error datasource_read_next(DataSource *source, DataRecord **record);
bool datasource_has_next(DataSource *source);
Error datasource_reset(DataSource *source);
void datasource_destroy(DataSource *source);

/* Schema helpers */
DataSchema* schema_create(int num_columns);
void schema_destroy(DataSchema *schema);
int schema_find_column(const DataSchema *schema, const char *name);
DataType schema_get_column_type(const DataSchema *schema, int index);

/* Record helpers */
DataRecord* record_create(int num_values);
void record_destroy(DataRecord *record);
float record_get_float(const DataRecord *record, int index);
int record_get_int(const DataRecord *record, int index);
const char* record_get_string(const DataRecord *record, int index);

#endif /* DATA_SOURCE_H */
