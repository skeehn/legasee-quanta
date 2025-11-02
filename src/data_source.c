#include "data_source.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Plugin registry */
#define MAX_PLUGINS 16

static struct {
    DataSourcePlugin plugins[MAX_PLUGINS];
    int count;
} g_plugin_registry = {0};

/* Register a data source plugin */
Error datasource_register_plugin(const char *type, DataSource* (*create_fn)(void)) {
    ERROR_CHECK_NULL(type, "Plugin type name");
    ERROR_CHECK_NULL(create_fn, "Plugin create function");

    if (g_plugin_registry.count >= MAX_PLUGINS) {
        return ERROR_CREATE(ERROR_OUT_OF_RESOURCES, "Plugin registry full");
    }

    /* Check for duplicate */
    for (int i = 0; i < g_plugin_registry.count; i++) {
        if (strcmp(g_plugin_registry.plugins[i].type_name, type) == 0) {
            return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Plugin already registered");
        }
    }

    g_plugin_registry.plugins[g_plugin_registry.count].type_name = type;
    g_plugin_registry.plugins[g_plugin_registry.count].create = create_fn;
    g_plugin_registry.count++;

    return (Error){SUCCESS};
}

/* Create data source by type */
DataSource* datasource_create(const char *type) {
    if (!type) return NULL;

    for (int i = 0; i < g_plugin_registry.count; i++) {
        if (strcmp(g_plugin_registry.plugins[i].type_name, type) == 0) {
            return g_plugin_registry.plugins[i].create();
        }
    }

    return NULL;
}

/* List all registered plugins */
void datasource_list_plugins(void) {
    printf("Registered data source plugins (%d):\n", g_plugin_registry.count);
    for (int i = 0; i < g_plugin_registry.count; i++) {
        printf("  - %s\n", g_plugin_registry.plugins[i].type_name);
    }
}

/* Data source wrapper functions */
Error datasource_init(DataSource *source, const char *config) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(source->interface, "Data source interface");

    if (!source->interface->init) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Init not implemented");
    }

    return source->interface->init(source, config);
}

Error datasource_open(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(source->interface, "Data source interface");

    if (!source->interface->open) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Open not implemented");
    }

    Error err = source->interface->open(source);
    if (err.code == SUCCESS) {
        source->is_open = true;
    }

    return err;
}

void datasource_close(DataSource *source) {
    if (!source || !source->interface || !source->interface->close) {
        return;
    }

    source->interface->close(source);
    source->is_open = false;
}

Error datasource_get_schema(DataSource *source, DataSchema **schema) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(schema, "Schema output");
    ERROR_CHECK_NULL(source->interface, "Data source interface");

    if (!source->interface->get_schema) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Get schema not implemented");
    }

    return source->interface->get_schema(source, schema);
}

Error datasource_read_next(DataSource *source, DataRecord **record) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(record, "Record output");
    ERROR_CHECK_NULL(source->interface, "Data source interface");

    if (!source->is_open) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Data source not open");
    }

    if (!source->interface->read_next) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Read next not implemented");
    }

    return source->interface->read_next(source, record);
}

bool datasource_has_next(DataSource *source) {
    if (!source || !source->interface || !source->interface->has_next) {
        return false;
    }

    return source->interface->has_next(source);
}

Error datasource_reset(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(source->interface, "Data source interface");

    if (!source->interface->reset) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Reset not supported");
    }

    return source->interface->reset(source);
}

void datasource_destroy(DataSource *source) {
    if (!source) return;

    if (source->is_open) {
        datasource_close(source);
    }

    if (source->interface && source->interface->destroy) {
        source->interface->destroy(source);
    }
}

/* Schema helpers */
DataSchema* schema_create(int num_columns) {
    if (num_columns <= 0) return NULL;

    DataSchema *schema = malloc(sizeof(DataSchema));
    if (!schema) return NULL;

    schema->columns = calloc(num_columns, sizeof(DataColumn));
    if (!schema->columns) {
        free(schema);
        return NULL;
    }

    schema->num_columns = num_columns;
    return schema;
}

void schema_destroy(DataSchema *schema) {
    if (!schema) return;

    if (schema->columns) {
        for (int i = 0; i < schema->num_columns; i++) {
            free(schema->columns[i].name);
        }
        free(schema->columns);
    }

    free(schema);
}

int schema_find_column(const DataSchema *schema, const char *name) {
    if (!schema || !name) return -1;

    for (int i = 0; i < schema->num_columns; i++) {
        if (schema->columns[i].name &&
            strcmp(schema->columns[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}

DataType schema_get_column_type(const DataSchema *schema, int index) {
    if (!schema || index < 0 || index >= schema->num_columns) {
        return DATA_TYPE_FLOAT;  /* Default */
    }

    return schema->columns[index].type;
}

/* Record helpers */
DataRecord* record_create(int num_values) {
    if (num_values <= 0) return NULL;

    DataRecord *record = malloc(sizeof(DataRecord));
    if (!record) return NULL;

    record->float_values = calloc(num_values, sizeof(float));
    record->int_values = calloc(num_values, sizeof(int));
    record->string_values = calloc(num_values, sizeof(char*));

    if (!record->float_values || !record->int_values || !record->string_values) {
        record_destroy(record);
        return NULL;
    }

    record->num_values = num_values;
    record->valid = true;

    return record;
}

void record_destroy(DataRecord *record) {
    if (!record) return;

    free(record->float_values);
    free(record->int_values);

    if (record->string_values) {
        for (int i = 0; i < record->num_values; i++) {
            free(record->string_values[i]);
        }
        free(record->string_values);
    }

    free(record);
}

float record_get_float(const DataRecord *record, int index) {
    if (!record || !record->float_values ||
        index < 0 || index >= record->num_values) {
        return 0.0f;
    }

    return record->float_values[index];
}

int record_get_int(const DataRecord *record, int index) {
    if (!record || !record->int_values ||
        index < 0 || index >= record->num_values) {
        return 0;
    }

    return record->int_values[index];
}

const char* record_get_string(const DataRecord *record, int index) {
    if (!record || !record->string_values ||
        index < 0 || index >= record->num_values) {
        return NULL;
    }

    return record->string_values[index];
}
