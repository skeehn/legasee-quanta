#include "csv_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Parse a CSV line into fields */
static int parse_csv_line(char *line, char **fields, int max_fields) {
    int field_count = 0;
    char *start = line;
    char *end;

    while (*start && field_count < max_fields) {
        /* Skip leading whitespace */
        while (isspace(*start)) start++;

        /* Find end of field (comma or newline) */
        end = start;
        while (*end && *end != ',' && *end != '\n' && *end != '\r') {
            end++;
        }

        /* Trim trailing whitespace */
        char *trim_end = end - 1;
        while (trim_end > start && isspace(*trim_end)) {
            trim_end--;
        }

        /* Copy field */
        size_t len = trim_end - start + 1;
        fields[field_count] = malloc(len + 1);
        if (!fields[field_count]) return -1;

        strncpy(fields[field_count], start, len);
        fields[field_count][len] = '\0';

        field_count++;

        /* Move to next field */
        if (*end == ',') {
            start = end + 1;
        } else {
            break;
        }
    }

    return field_count;
}

/* Load CSV file */
Error csv_load(const char *filename, CSVData **csv_out) {
    ERROR_CHECK_NULL(filename, "Filename");
    ERROR_CHECK_NULL(csv_out, "CSV output pointer");

    FILE *file = fopen(filename, "r");
    if (!file) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to open CSV file");
    }

    CSVData *csv = malloc(sizeof(CSVData));
    if (!csv) {
        fclose(file);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate CSV structure");
    }

    csv->headers = NULL;
    csv->data = NULL;
    csv->num_rows = 0;
    csv->num_columns = 0;

    char line[CSV_MAX_LINE];
    char *fields[CSV_MAX_COLUMNS];

    /* Read header line */
    if (!fgets(line, sizeof(line), file)) {
        fclose(file);
        free(csv);
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to read CSV header");
    }

    int num_columns = parse_csv_line(line, fields, CSV_MAX_COLUMNS);
    if (num_columns <= 0) {
        fclose(file);
        free(csv);
        return ERROR_CREATE(ERROR_INVALID_PARAMETER, "Invalid CSV header");
    }

    /* Allocate headers */
    csv->num_columns = num_columns;
    csv->headers = malloc(num_columns * sizeof(char*));
    if (!csv->headers) {
        fclose(file);
        free(csv);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate headers");
    }

    /* Copy headers */
    for (int i = 0; i < num_columns; i++) {
        csv->headers[i] = fields[i];
    }

    /* Allocate data array */
    csv->data = malloc(CSV_MAX_ROWS * sizeof(float*));
    if (!csv->data) {
        for (int i = 0; i < num_columns; i++) {
            free(csv->headers[i]);
        }
        free(csv->headers);
        fclose(file);
        free(csv);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate data array");
    }

    /* Read data lines */
    int row = 0;
    while (fgets(line, sizeof(line), file) && row < CSV_MAX_ROWS) {
        /* Skip empty lines */
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '\0') {
            continue;
        }

        int field_count = parse_csv_line(line, fields, CSV_MAX_COLUMNS);
        if (field_count != num_columns) {
            /* Skip malformed lines */
            for (int i = 0; i < field_count; i++) {
                free(fields[i]);
            }
            continue;
        }

        /* Allocate row */
        csv->data[row] = malloc(num_columns * sizeof(float));
        if (!csv->data[row]) {
            /* Cleanup on error */
            for (int i = 0; i < field_count; i++) {
                free(fields[i]);
            }
            break;
        }

        /* Convert to floats */
        for (int col = 0; col < num_columns; col++) {
            csv->data[row][col] = atof(fields[col]);
            free(fields[col]);
        }

        row++;
    }

    csv->num_rows = row;
    fclose(file);

    *csv_out = csv;
    return (Error){SUCCESS};
}

/* Free CSV data */
void csv_free(CSVData *csv) {
    if (!csv) return;

    if (csv->headers) {
        for (int i = 0; i < csv->num_columns; i++) {
            free(csv->headers[i]);
        }
        free(csv->headers);
    }

    if (csv->data) {
        for (int i = 0; i < csv->num_rows; i++) {
            free(csv->data[i]);
        }
        free(csv->data);
    }

    free(csv);
}

/* Get value at row, column */
float csv_get_value(const CSVData *csv, int row, int column) {
    if (!csv || row < 0 || row >= csv->num_rows ||
        column < 0 || column >= csv->num_columns) {
        return 0.0f;
    }
    return csv->data[row][column];
}

/* Get header name */
const char* csv_get_header(const CSVData *csv, int column) {
    if (!csv || column < 0 || column >= csv->num_columns) {
        return NULL;
    }
    return csv->headers[column];
}

/* Find column by name */
int csv_find_column(const CSVData *csv, const char *header_name) {
    if (!csv || !header_name) return -1;

    for (int i = 0; i < csv->num_columns; i++) {
        if (strcmp(csv->headers[i], header_name) == 0) {
            return i;
        }
    }
    return -1;
}

/* Print CSV information */
void csv_print_info(const CSVData *csv) {
    if (!csv) {
        printf("CSV: NULL\n");
        return;
    }

    printf("CSV Information:\n");
    printf("  Rows: %d\n", csv->num_rows);
    printf("  Columns: %d\n", csv->num_columns);
    printf("  Headers: ");
    for (int i = 0; i < csv->num_columns; i++) {
        printf("%s", csv->headers[i]);
        if (i < csv->num_columns - 1) printf(", ");
    }
    printf("\n");
}

/* Print CSV data */
void csv_print_data(const CSVData *csv, int max_rows) {
    if (!csv) return;

    /* Print headers */
    for (int i = 0; i < csv->num_columns; i++) {
        printf("%-12s ", csv->headers[i]);
    }
    printf("\n");

    for (int i = 0; i < csv->num_columns; i++) {
        printf("------------ ");
    }
    printf("\n");

    /* Print data */
    int rows_to_print = (max_rows < csv->num_rows) ? max_rows : csv->num_rows;
    for (int row = 0; row < rows_to_print; row++) {
        for (int col = 0; col < csv->num_columns; col++) {
            printf("%-12.2f ", csv->data[row][col]);
        }
        printf("\n");
    }

    if (max_rows < csv->num_rows) {
        printf("... (%d more rows)\n", csv->num_rows - max_rows);
    }
}
