#ifndef CSV_LOADER_H
#define CSV_LOADER_H

#include <stddef.h>
#include "error.h"

/* Maximum columns and rows for the prototype */
#define CSV_MAX_COLUMNS 10
#define CSV_MAX_ROWS 1000
#define CSV_MAX_LINE 1024

/* CSV data structure */
typedef struct {
    char **headers;         /* Column names */
    float **data;           /* 2D array: [row][column] */
    int num_rows;
    int num_columns;
} CSVData;

/* CSV loading functions */
Error csv_load(const char *filename, CSVData **csv_out);
void csv_free(CSVData *csv);

/* Data access helpers */
float csv_get_value(const CSVData *csv, int row, int column);
const char* csv_get_header(const CSVData *csv, int column);
int csv_find_column(const CSVData *csv, const char *header_name);

/* Utility functions */
void csv_print_info(const CSVData *csv);
void csv_print_data(const CSVData *csv, int max_rows);

#endif /* CSV_LOADER_H */
