#include "ai.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

/* ===== UTILITY FUNCTIONS ===== */

float ai_mean(const float *data, int size) {
    if (size <= 0) return 0.0f;

    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        sum += data[i];
    }
    return sum / size;
}

float ai_stddev(const float *data, int size) {
    if (size <= 1) return 0.0f;

    float mean = ai_mean(data, size);
    float sum_sq = 0.0f;

    for (int i = 0; i < size; i++) {
        float diff = data[i] - mean;
        sum_sq += diff * diff;
    }

    return sqrtf(sum_sq / (size - 1));
}

/* Comparison function for qsort */
static int compare_floats(const void *a, const void *b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

float ai_median(const float *data, int size) {
    if (size <= 0) return 0.0f;

    /* Copy data for sorting */
    float *sorted = malloc(sizeof(float) * size);
    if (!sorted) return 0.0f;

    memcpy(sorted, data, sizeof(float) * size);
    qsort(sorted, size, sizeof(float), compare_floats);

    float result;
    if (size % 2 == 0) {
        result = (sorted[size/2 - 1] + sorted[size/2]) / 2.0f;
    } else {
        result = sorted[size/2];
    }

    free(sorted);
    return result;
}

float ai_moving_average(const float *data, int size, int window) {
    if (size <= 0 || window <= 0) return 0.0f;

    int start = size - window;
    if (start < 0) start = 0;

    float sum = 0.0f;
    int count = 0;

    for (int i = start; i < size; i++) {
        sum += data[i];
        count++;
    }

    return count > 0 ? sum / count : 0.0f;
}

/* ===== ANOMALY DETECTION ===== */

int ai_detect_anomalies_zscore(const float *data, int size,
                                float threshold,
                                AnomalyResult *results, int max_results) {
    if (!data || !results || size <= 0 || max_results <= 0) return 0;

    float mean = ai_mean(data, size);
    float stddev = ai_stddev(data, size);

    if (stddev < 0.0001f) return 0;  /* No variance */

    int anomaly_count = 0;

    for (int i = 0; i < size && anomaly_count < max_results; i++) {
        float z_score = fabsf((data[i] - mean) / stddev);

        if (z_score > threshold) {
            results[anomaly_count].index = i;
            results[anomaly_count].value = data[i];
            results[anomaly_count].expected = mean;
            results[anomaly_count].deviation = z_score;
            results[anomaly_count].is_anomaly = true;
            anomaly_count++;
        }
    }

    return anomaly_count;
}

int ai_detect_anomalies_moving_avg(const float *data, int size,
                                    int window_size, float threshold,
                                    AnomalyResult *results, int max_results) {
    if (!data || !results || size <= 0 || max_results <= 0) return 0;
    if (window_size <= 0 || window_size > size) window_size = size / 4;

    int anomaly_count = 0;

    for (int i = window_size; i < size && anomaly_count < max_results; i++) {
        /* Calculate moving average of previous window */
        float moving_avg = ai_moving_average(data + i - window_size, window_size, window_size);

        float deviation = fabsf(data[i] - moving_avg);

        if (deviation > threshold) {
            results[anomaly_count].index = i;
            results[anomaly_count].value = data[i];
            results[anomaly_count].expected = moving_avg;
            results[anomaly_count].deviation = deviation;
            results[anomaly_count].is_anomaly = true;
            anomaly_count++;
        }
    }

    return anomaly_count;
}

/* ===== K-MEANS CLUSTERING ===== */

/* Calculate Euclidean distance */
static float euclidean_distance(float x1, float y1, float x2, float y2) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrtf(dx * dx + dy * dy);
}

Error ai_kmeans_cluster(const float *x, const float *y, const float *values,
                        int num_points, int k, int max_iterations,
                        ClusteringResult *result) {
    ERROR_CHECK_NULL(x, "X coordinates");
    ERROR_CHECK_NULL(y, "Y coordinates");
    ERROR_CHECK_NULL(result, "Result");
    ERROR_CHECK(num_points > 0, ERROR_INVALID_PARAMETER, "Need at least 1 point");
    ERROR_CHECK(k > 0 && k <= AI_MAX_CLUSTERS, ERROR_INVALID_PARAMETER, "Invalid k");

    /* Initialize result */
    memset(result, 0, sizeof(ClusteringResult));
    result->num_clusters = k;
    result->num_points = num_points;

    /* Allocate assignments */
    result->assignments = malloc(sizeof(int) * num_points);
    if (!result->assignments) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate assignments");
    }

    /* Initialize centroids (k-means++ initialization would be better, but this is simpler) */
    for (int i = 0; i < k; i++) {
        int idx = rand() % num_points;
        result->clusters[i].centroid_x = x[idx];
        result->clusters[i].centroid_y = y[idx];
    }

    /* K-means iterations */
    bool changed = true;
    int iteration = 0;

    while (changed && iteration < max_iterations) {
        changed = false;

        /* Assignment step */
        for (int i = 0; i < num_points; i++) {
            float min_dist = INFINITY;
            int best_cluster = 0;

            for (int j = 0; j < k; j++) {
                float dist = euclidean_distance(x[i], y[i],
                                               result->clusters[j].centroid_x,
                                               result->clusters[j].centroid_y);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = j;
                }
            }

            if (result->assignments[i] != best_cluster) {
                result->assignments[i] = best_cluster;
                changed = true;
            }
        }

        /* Update step */
        for (int j = 0; j < k; j++) {
            float sum_x = 0.0f, sum_y = 0.0f, sum_val = 0.0f;
            int count = 0;

            for (int i = 0; i < num_points; i++) {
                if (result->assignments[i] == j) {
                    sum_x += x[i];
                    sum_y += y[i];
                    if (values) sum_val += values[i];
                    count++;
                }
            }

            if (count > 0) {
                result->clusters[j].centroid_x = sum_x / count;
                result->clusters[j].centroid_y = sum_y / count;
                result->clusters[j].avg_value = values ? sum_val / count : 0.0f;
                result->clusters[j].count = count;
            }
        }

        iteration++;
    }

    /* Calculate inertia (sum of squared distances) */
    result->inertia = 0.0f;
    for (int i = 0; i < num_points; i++) {
        int cluster_id = result->assignments[i];
        float dist = euclidean_distance(x[i], y[i],
                                       result->clusters[cluster_id].centroid_x,
                                       result->clusters[cluster_id].centroid_y);
        result->inertia += dist * dist;
    }

    return (Error){SUCCESS};
}

void ai_clustering_result_free(ClusteringResult *result) {
    if (result && result->assignments) {
        free(result->assignments);
        result->assignments = NULL;
    }
}

/* ===== TIME SERIES PREDICTION ===== */

Prediction ai_predict_linear(const float *data, int size, int steps_ahead) {
    Prediction pred = {0};

    if (!data || size < 2) {
        return pred;
    }

    /* Simple linear regression: y = mx + b */
    float sum_x = 0.0f, sum_y = 0.0f, sum_xy = 0.0f, sum_xx = 0.0f;

    for (int i = 0; i < size; i++) {
        float x = (float)i;
        float y = data[i];
        sum_x += x;
        sum_y += y;
        sum_xy += x * y;
        sum_xx += x * x;
    }

    float n = (float)size;
    float slope = (n * sum_xy - sum_x * sum_y) / (n * sum_xx - sum_x * sum_x);
    float intercept = (sum_y - slope * sum_x) / n;

    /* Predict */
    float next_x = (float)(size + steps_ahead - 1);
    pred.value = slope * next_x + intercept;

    /* Calculate confidence based on residuals */
    float sse = 0.0f;
    for (int i = 0; i < size; i++) {
        float predicted = slope * i + intercept;
        float residual = data[i] - predicted;
        sse += residual * residual;
    }

    float std_error = sqrtf(sse / (size - 2));
    pred.confidence = 1.0f / (1.0f + std_error);  /* Simple heuristic */
    pred.lower_bound = pred.value - 2.0f * std_error;
    pred.upper_bound = pred.value + 2.0f * std_error;

    return pred;
}

Prediction ai_predict_exponential(const float *data, int size, float alpha) {
    Prediction pred = {0};

    if (!data || size < 1) {
        return pred;
    }

    if (alpha < 0.0f) alpha = 0.3f;  /* Default smoothing */
    if (alpha > 1.0f) alpha = 1.0f;

    /* Exponential smoothing */
    float smoothed = data[0];

    for (int i = 1; i < size; i++) {
        smoothed = alpha * data[i] + (1.0f - alpha) * smoothed;
    }

    pred.value = smoothed;

    /* Calculate confidence based on recent variance */
    float variance = 0.0f;
    int recent = size < 10 ? size : 10;
    for (int i = size - recent; i < size; i++) {
        float diff = data[i] - smoothed;
        variance += diff * diff;
    }
    variance /= recent;

    pred.confidence = 1.0f / (1.0f + sqrtf(variance));
    pred.lower_bound = pred.value - sqrtf(variance);
    pred.upper_bound = pred.value + sqrtf(variance);

    return pred;
}

/* ===== NATURAL LANGUAGE QUERIES ===== */

/* Check if string matches keyword */
static bool is_keyword(const char *str) {
    return strcmp(str, "where") == 0 ||
           strcmp(str, "and") == 0 ||
           strcmp(str, "or") == 0 ||
           strcmp(str, "not") == 0;
}

/* Check if string is a field name */
static bool is_field(const char *str) {
    return strcmp(str, "x") == 0 ||
           strcmp(str, "y") == 0 ||
           strcmp(str, "value") == 0 ||
           strcmp(str, "speed") == 0;
}

/* Check if string is an operator */
static bool is_operator(const char *str) {
    return strcmp(str, ">") == 0 ||
           strcmp(str, "<") == 0 ||
           strcmp(str, "=") == 0 ||
           strcmp(str, "!=") == 0 ||
           strcmp(str, ">=") == 0 ||
           strcmp(str, "<=") == 0;
}

int ai_parse_query(const char *query, QueryToken *tokens, int max_tokens) {
    if (!query || !tokens || max_tokens <= 0) return 0;

    int token_count = 0;
    const char *p = query;

    while (*p && token_count < max_tokens) {
        /* Skip whitespace */
        while (*p && isspace(*p)) p++;
        if (!*p) break;

        /* Parse token */
        char token_text[64] = {0};
        int len = 0;

        /* Handle operators */
        if (strchr("><=!", *p)) {
            token_text[len++] = *p++;
            if (*p == '=') {
                token_text[len++] = *p++;
            }
            token_text[len] = '\0';

            strncpy(tokens[token_count].text, token_text, 63);
            tokens[token_count].type = TOKEN_OPERATOR;
            token_count++;
            continue;
        }

        /* Handle numbers */
        if (isdigit(*p) || (*p == '-' && isdigit(*(p+1)))) {
            char *end;
            float num = strtof(p, &end);
            tokens[token_count].number = num;
            tokens[token_count].type = TOKEN_NUMBER;
            snprintf(tokens[token_count].text, 64, "%g", num);
            token_count++;
            p = end;
            continue;
        }

        /* Handle words */
        while (*p && (isalnum(*p) || *p == '_') && len < 63) {
            token_text[len++] = tolower(*p);
            p++;
        }
        token_text[len] = '\0';

        if (len > 0) {
            strncpy(tokens[token_count].text, token_text, 63);

            if (is_keyword(token_text)) {
                tokens[token_count].type = TOKEN_KEYWORD;
            } else if (is_field(token_text)) {
                tokens[token_count].type = TOKEN_FIELD;
            } else {
                tokens[token_count].type = TOKEN_STRING;
            }

            token_count++;
        } else {
            p++;  /* Skip unknown character */
        }
    }

    return token_count;
}

bool ai_eval_query(const QueryToken *tokens, int num_tokens,
                    float x, float y, float value) {
    if (!tokens || num_tokens == 0) return true;

    /* Very simple evaluation: "field operator number" */
    /* Example: "x > 50" or "where x > 50 and value < 100" */

    bool result = true;
    bool use_and = true;

    for (int i = 0; i < num_tokens; i++) {
        /* Skip "where" keyword */
        if (tokens[i].type == TOKEN_KEYWORD &&
            strcmp(tokens[i].text, "where") == 0) {
            continue;
        }

        /* Handle AND/OR */
        if (tokens[i].type == TOKEN_KEYWORD) {
            if (strcmp(tokens[i].text, "and") == 0) {
                use_and = true;
            } else if (strcmp(tokens[i].text, "or") == 0) {
                use_and = false;
            }
            continue;
        }

        /* Parse simple condition: field operator value */
        if (i + 2 < num_tokens &&
            tokens[i].type == TOKEN_FIELD &&
            tokens[i+1].type == TOKEN_OPERATOR &&
            tokens[i+2].type == TOKEN_NUMBER) {

            float field_value = 0.0f;
            if (strcmp(tokens[i].text, "x") == 0) field_value = x;
            else if (strcmp(tokens[i].text, "y") == 0) field_value = y;
            else if (strcmp(tokens[i].text, "value") == 0) field_value = value;

            float threshold = tokens[i+2].number;
            bool condition = false;

            const char *op = tokens[i+1].text;
            if (strcmp(op, ">") == 0) condition = field_value > threshold;
            else if (strcmp(op, "<") == 0) condition = field_value < threshold;
            else if (strcmp(op, ">=") == 0) condition = field_value >= threshold;
            else if (strcmp(op, "<=") == 0) condition = field_value <= threshold;
            else if (strcmp(op, "=") == 0) condition = fabsf(field_value - threshold) < 0.001f;
            else if (strcmp(op, "!=") == 0) condition = fabsf(field_value - threshold) >= 0.001f;

            /* Combine with previous result */
            if (use_and) {
                result = result && condition;
            } else {
                result = result || condition;
            }

            i += 2;  /* Skip operator and number */
        }
    }

    return result;
}
