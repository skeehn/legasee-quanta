#ifndef JSON_DATASOURCE_H
#define JSON_DATASOURCE_H

#include "data_source.h"

/* Register JSON data source plugin */
void json_datasource_register(void);

/* Create JSON data source directly */
DataSource* json_datasource_create(void);

#endif /* JSON_DATASOURCE_H */
