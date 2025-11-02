#include "json_datasource.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

/* Simple JSON parser for array-of-objects format */
/* Example: [{"x":10,"y":5,"value":100}, {"x":20,"y":8,"value":85}] */

#define MAX_JSON_RECORDS 1000
#define MAX_JSON_FIELDS 20
#define MAX_FIELD_NAME 64
#define MAX_JSON_SIZE (1024 * 1024)  /* 1MB max */

typedef struct {
    char name[MAX_FIELD_NAME];
    float value;
} JSONField;

typedef struct {
    JSONField fields[MAX_JSON_FIELDS];
    int num_fields;
} JSONRecord;

typedef struct {
    JSONRecord records[MAX_JSON_RECORDS];
    int num_records;
    char field_names[MAX_JSON_FIELDS][MAX_FIELD_NAME];
    int num_fields;
    int current_record;
    char *filename;
} JSONSourceData;

/* Skip whitespace */
static const char* skip_whitespace(const char *p) {
    while (*p && isspace(*p)) p++;
    return p;
}

/* Parse string value */
static const char* parse_string(const char *p, char *out, int max_len) {
    if (*p != '"') return NULL;
    p++;  /* Skip opening quote */

    int i = 0;
    while (*p && *p != '"' && i < max_len - 1) {
        out[i++] = *p++;
    }
    out[i] = '\0';

    if (*p != '"') return NULL;
    return p + 1;  /* Skip closing quote */
}

/* Parse number */
static const char* parse_number(const char *p, float *out) {
    char *end;
    *out = strtof(p, &end);
    return (end != p) ? end : NULL;
}

/* Parse object */
static const char* parse_object(const char *p, JSONRecord *record) {
    p = skip_whitespace(p);
    if (*p != '{') return NULL;
    p++;  /* Skip '{' */

    record->num_fields = 0;

    while (1) {
        p = skip_whitespace(p);
        if (*p == '}') {
            return p + 1;  /* End of object */
        }

        if (record->num_fields >= MAX_JSON_FIELDS) break;

        /* Parse field name */
        char field_name[MAX_FIELD_NAME];
        p = parse_string(p, field_name, MAX_FIELD_NAME);
        if (!p) return NULL;

        p = skip_whitespace(p);
        if (*p != ':') return NULL;
        p++;  /* Skip ':' */

        p = skip_whitespace(p);

        /* Parse value (number or string) */
        float value;
        if (*p == '"') {
            /* String value - store as 0 for now */
            char str_value[256];
            p = parse_string(p, str_value, sizeof(str_value));
            if (!p) return NULL;
            value = 0.0f;  /* TODO: handle strings */
        } else {
            /* Number value */
            p = parse_number(p, &value);
            if (!p) return NULL;
        }

        /* Store field */
        strncpy(record->fields[record->num_fields].name, field_name, MAX_FIELD_NAME - 1);
        record->fields[record->num_fields].value = value;
        record->num_fields++;

        p = skip_whitespace(p);
        if (*p == ',') {
            p++;  /* Skip ',' */
        } else if (*p == '}') {
            return p + 1;
        } else {
            return NULL;
        }
    }

    return NULL;
}

/* Parse array of objects */
static int parse_json_array(const char *json, JSONSourceData *data) {
    const char *p = skip_whitespace(json);

    if (*p != '[') return -1;
    p++;  /* Skip '[' */

    data->num_records = 0;

    while (1) {
        p = skip_whitespace(p);
        if (*p == ']') break;  /* End of array */

        if (data->num_records >= MAX_JSON_RECORDS) break;

        /* Parse object */
        p = parse_object(p, &data->records[data->num_records]);
        if (!p) return -1;

        data->num_records++;

        p = skip_whitespace(p);
        if (*p == ',') {
            p++;  /* Skip ',' */
        } else if (*p == ']') {
            break;
        }
    }

    /* Extract unique field names from first record */
    if (data->num_records > 0) {
        data->num_fields = data->records[0].num_fields;
        for (int i = 0; i < data->num_fields && i < MAX_JSON_FIELDS; i++) {
            strncpy(data->field_names[i],
                   data->records[0].fields[i].name,
                   MAX_FIELD_NAME - 1);
        }
    }

    return data->num_records;
}

/* Data source interface implementations */
static Error json_init(DataSource *source, const char *config) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(config, "Config (filename)");

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data) {
        return ERROR_CREATE(ERROR_NULL_POINTER, "Private data not initialized");
    }

    data->filename = strdup(config);
    if (!data->filename) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to copy filename");
    }

    return (Error){SUCCESS};
}

static Error json_open(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data || !data->filename) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "No filename configured");
    }

    /* Read entire file */
    FILE *file = fopen(data->filename, "r");
    if (!file) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open JSON file");
    }

    /* Get file size */
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size > MAX_JSON_SIZE) {
        fclose(file);
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, "JSON file too large");
    }

    /* Read file */
    char *json = malloc(size + 1);
    if (!json) {
        fclose(file);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate JSON buffer");
    }

    size_t read_size = fread(json, 1, size, file);
    json[read_size] = '\0';
    fclose(file);

    /* Parse JSON */
    int num_records = parse_json_array(json, data);
    free(json);

    if (num_records < 0) {
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Failed to parse JSON");
    }

    data->current_record = 0;
    return (Error){SUCCESS};
}

static void json_close(DataSource *source) {
    if (!source) return;

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (data) {
        data->num_records = 0;
        data->current_record = 0;
    }
}

static Error json_get_schema(DataSource *source, DataSchema **schema_out) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(schema_out, "Schema output");

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data || data->num_records == 0) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "JSON data not loaded");
    }

    DataSchema *schema = schema_create(data->num_fields);
    if (!schema) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to create schema");
    }

    for (int i = 0; i < data->num_fields; i++) {
        schema->columns[i].name = strdup(data->field_names[i]);
        schema->columns[i].type = DATA_TYPE_FLOAT;
        schema->columns[i].index = i;
    }

    *schema_out = schema;
    return (Error){SUCCESS};
}

static Error json_read_next(DataSource *source, DataRecord **record_out) {
    ERROR_CHECK_NULL(source, "Data source");
    ERROR_CHECK_NULL(record_out, "Record output");

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data || data->num_records == 0) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "JSON data not loaded");
    }

    if (data->current_record >= data->num_records) {
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, "No more records");
    }

    DataRecord *record = record_create(data->num_fields);
    if (!record) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to create record");
    }

    /* Copy data */
    JSONRecord *json_record = &data->records[data->current_record];
    for (int i = 0; i < data->num_fields && i < json_record->num_fields; i++) {
        record->float_values[i] = json_record->fields[i].value;
    }

    data->current_record++;
    *record_out = record;

    return (Error){SUCCESS};
}

static bool json_has_next(DataSource *source) {
    if (!source) return false;

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data) return false;

    return data->current_record < data->num_records;
}

static Error json_reset(DataSource *source) {
    ERROR_CHECK_NULL(source, "Data source");

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (!data) {
        return ERROR_CREATE(ERROR_NULL_POINTER, "Private data not initialized");
    }

    data->current_record = 0;
    return (Error){SUCCESS};
}

static uint32_t json_get_capabilities(DataSource *source) {
    (void)source;
    return CAP_SEEKABLE | CAP_RANDOM | CAP_BUFFERED;
}

static void json_destroy(DataSource *source) {
    if (!source) return;

    JSONSourceData *data = (JSONSourceData*)source->private_data;
    if (data) {
        free(data->filename);
        free(data);
    }

    free(source);
}

/* Interface */
static DataSourceInterface json_interface = {
    .init = json_init,
    .open = json_open,
    .close = json_close,
    .get_schema = json_get_schema,
    .read_next = json_read_next,
    .has_next = json_has_next,
    .reset = json_reset,
    .get_capabilities = json_get_capabilities,
    .destroy = json_destroy
};

/* Create JSON data source */
DataSource* json_datasource_create(void) {
    DataSource *source = malloc(sizeof(DataSource));
    if (!source) return NULL;

    JSONSourceData *data = calloc(1, sizeof(JSONSourceData));
    if (!data) {
        free(source);
        return NULL;
    }

    source->name = "JSON File";
    source->type = "json";
    source->interface = &json_interface;
    source->private_data = data;
    source->schema = NULL;
    source->capabilities = CAP_SEEKABLE | CAP_RANDOM | CAP_BUFFERED;
    source->is_open = false;

    return source;
}

/* Register JSON plugin */
void json_datasource_register(void) {
    datasource_register_plugin("json", json_datasource_create);
}
