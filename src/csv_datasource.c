#include "csv_datasource.h"
#include "csv_loader.h"
#include <stdlib.h>
#include <string.h>

/* CSV data source private data */
typedef struct {
    CSVData *csv_data;
    char *filename;
    int current_row;
} CSVSourceData;

/* Forward declarations */
static Error csv_init(DataSource *source, const char *config);
static Error csv_open(DataSource *source);
static void csv_close(DataSource *source);
static Error csv_get_schema(DataSource *source, DataSchema **schema);
static Error csv_read_next(DataSource *source, DataRecord **record);
static bool csv_has_next(DataSource *source);
static Error csv_reset(DataSource *source);
static uint32_t csv_get_capabilities(DataSource *source);
static void csv_destroy(DataSource *source);

/* Interface implementation */
static DataSourceInterface csv_interface = {
    .init = csv_init,
    .open = csv_open,
    .close = csv_close,
    .get_schema = csv_get_schema,
    .read_next = csv_read_next,
    .has_next = csv_has_next,
    .reset = csv_reset,
    .get_capabilities = csv_get_capabilities,
    .destroy = csv_destroy
};

/* Create CSV data source */
DataSource* csv_datasource_create(void) {
    DataSource *source = malloc(sizeof(DataSource));
    if (!source) return NULL;

    CSVSourceData *data = malloc(sizeof(CSVSourceData));
    if (!data) {
        free(source);
        return NULL;
    }

    data->csv_data = NULL;
    data->filename = NULL;
    data->current_row = 0;

    source->name = "CSV File";
    source->type = "csv";
    source->interface = &csv_interface;
    source->private_data = data;
    source->schema = NULL;
    source->capabilities = CAP_SEEKABLE | CAP_RANDOM | CAP_BUFFERED;
    source->is_open = false;

    return source;
}

/* Initialize with config (filename) */
static Error csv_init(DataSource *source, const char *config) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(config, "Config (filename)");

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data) {
        return ERROR_CREATE(ERROR_NULL_POINTER, "Private data not initialized");
    }

    /* Store filename */
    data->filename = strdup(config);
    if (!data->filename) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to copy filename");
    }

    return (Error){SUCCESS};
}

/* Open CSV file */
static Error csv_open(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data || !data->filename) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "No filename configured");
    }

    /* Load CSV file */
    Error err = csv_load(data->filename, &data->csv_data);
    if (err.code != SUCCESS) {
        return err;
    }

    data->current_row = 0;
    return (Error){SUCCESS};
}

/* Close CSV file */
static void csv_close(DataSource *source) {
    if (!source) return;

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data) return;

    if (data->csv_data) {
        csv_free(data->csv_data);
        data->csv_data = NULL;
    }

    data->current_row = 0;
}

/* Get schema */
static Error csv_get_schema(DataSource *source, DataSchema **schema_out) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(schema_out, "Schema output");

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data || !data->csv_data) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "CSV data not loaded");
    }

    /* Create schema from CSV headers */
    DataSchema *schema = schema_create(data->csv_data->num_columns);
    if (!schema) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to create schema");
    }

    for (int i = 0; i < data->csv_data->num_columns; i++) {
        schema->columns[i].name = strdup(data->csv_data->headers[i]);
        schema->columns[i].type = DATA_TYPE_FLOAT;  /* CSV stores all as floats */
        schema->columns[i].index = i;
    }

    *schema_out = schema;
    return (Error){SUCCESS};
}

/* Read next record */
static Error csv_read_next(DataSource *source, DataRecord **record_out) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(record_out, "Record output");

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data || !data->csv_data) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "CSV data not loaded");
    }

    if (data->current_row >= data->csv_data->num_rows) {
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, "No more records");
    }

    /* Create record */
    DataRecord *record = record_create(data->csv_data->num_columns);
    if (!record) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to create record");
    }

    /* Copy data from CSV */
    for (int i = 0; i < data->csv_data->num_columns; i++) {
        record->float_values[i] = data->csv_data->data[data->current_row][i];
    }

    data->current_row++;
    *record_out = record;

    return (Error){SUCCESS};
}

/* Check if more data available */
static bool csv_has_next(DataSource *source) {
    if (!source) return false;

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data || !data->csv_data) return false;

    return data->current_row < data->csv_data->num_rows;
}

/* Reset to beginning */
static Error csv_reset(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (!data) {
        return ERROR_CREATE(ERROR_NULL_POINTER, "Private data not initialized");
    }

    data->current_row = 0;
    return (Error){SUCCESS};
}

/* Get capabilities */
static uint32_t csv_get_capabilities(DataSource *source) {
    (void)source;  /* Unused */
    return CAP_SEEKABLE | CAP_RANDOM | CAP_BUFFERED;
}

/* Destroy data source */
static void csv_destroy(DataSource *source) {
    if (!source) return;

    CSVSourceData *data = (CSVSourceData*)source->private_data;
    if (data) {
        if (data->csv_data) {
            csv_free(data->csv_data);
        }
        free(data->filename);
        free(data);
    }

    free(source);
}

/* Register CSV plugin */
void csv_datasource_register(void) {
    datasource_register_plugin("csv", csv_datasource_create);
}
