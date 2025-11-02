#ifndef CSV_DATASOURCE_H
#define CSV_DATASOURCE_H

#include "data_source.h"

/* Register CSV data source plugin */
void csv_datasource_register(void);

/* Create CSV data source directly */
DataSource* csv_datasource_create(void);

#endif /* CSV_DATASOURCE_H */
