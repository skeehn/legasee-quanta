#ifndef AI_H
#define AI_H

#include <stdint.h>
#include <stdbool.h>
#include "error.h"

/**
 * AI Features Module (Week 4)
 *
 * Lightweight AI algorithms for particle data analysis:
 * - Anomaly Detection: Statistical outlier detection
 * - Clustering: K-means algorithm for pattern recognition
 * - Prediction: Time-series forecasting
 * - NLP Queries: Simple natural language data filtering
 *
 * All algorithms are optimized for real-time performance
 * with minimal memory footprint.
 */

#define AI_MAX_CLUSTERS 16
#define AI_MAX_QUERY_TOKENS 32
#define AI_MAX_TIME_SERIES 1000

/* Anomaly detection methods */
typedef enum {
    ANOMALY_ZSCORE,         /* Z-score based (statistical) */
    ANOMALY_IQR,            /* Interquartile range */
    ANOMALY_MOVING_AVG      /* Moving average deviation */
} AnomalyMethod;

/* Anomaly result */
typedef struct {
    int index;              /* Data point index */
    float value;            /* Actual value */
    float expected;         /* Expected value */
    float deviation;        /* Deviation score */
    bool is_anomaly;        /* Whether this is an anomaly */
} AnomalyResult;

/* Cluster (K-means) */
typedef struct {
    float centroid_x;       /* Cluster center X */
    float centroid_y;       /* Cluster center Y */
    int count;              /* Number of points in cluster */
    float avg_value;        /* Average value in cluster */
} Cluster;

/* Clustering result */
typedef struct {
    Cluster clusters[AI_MAX_CLUSTERS];
    int num_clusters;
    int *assignments;       /* Point -> cluster mapping */
    int num_points;
    float inertia;          /* Sum of squared distances */
} ClusteringResult;

/* Time series prediction */
typedef struct {
    float value;            /* Predicted value */
    float confidence;       /* Confidence (0-1) */
    float lower_bound;      /* Confidence interval lower */
    float upper_bound;      /* Confidence interval upper */
} Prediction;

/* Query token types */
typedef enum {
    TOKEN_KEYWORD,          /* where, and, or, not */
    TOKEN_FIELD,            /* x, y, value, speed */
    TOKEN_OPERATOR,         /* >, <, =, != */
    TOKEN_NUMBER,           /* numeric literal */
    TOKEN_STRING            /* string literal */
} TokenType;

/* Query token */
typedef struct {
    TokenType type;
    char text[64];
    float number;
} QueryToken;

/**
 * ANOMALY DETECTION
 */

/**
 * Detect anomalies in 1D data using Z-score
 *
 * @param data Input data array
 * @param size Number of data points
 * @param threshold Z-score threshold (typically 2.0-3.0)
 * @param results Output anomaly results
 * @param max_results Maximum results to return
 * @return Number of anomalies found
 */
int ai_detect_anomalies_zscore(const float *data, int size,
                                float threshold,
                                AnomalyResult *results, int max_results);

/**
 * Detect anomalies using moving average
 *
 * @param data Input data array
 * @param size Number of data points
 * @param window_size Moving average window
 * @param threshold Deviation threshold
 * @param results Output anomaly results
 * @param max_results Maximum results to return
 * @return Number of anomalies found
 */
int ai_detect_anomalies_moving_avg(const float *data, int size,
                                    int window_size, float threshold,
                                    AnomalyResult *results, int max_results);

/**
 * K-MEANS CLUSTERING
 */

/**
 * Perform K-means clustering on 2D points
 *
 * @param x X coordinates
 * @param y Y coordinates
 * @param values Associated values (can be NULL)
 * @param num_points Number of points
 * @param k Number of clusters
 * @param max_iterations Maximum iterations
 * @param result Output clustering result
 * @return Error status
 */
Error ai_kmeans_cluster(const float *x, const float *y, const float *values,
                        int num_points, int k, int max_iterations,
                        ClusteringResult *result);

/**
 * Free clustering result
 */
void ai_clustering_result_free(ClusteringResult *result);

/**
 * TIME SERIES PREDICTION
 */

/**
 * Predict next value using linear regression
 *
 * @param data Historical data
 * @param size Number of historical points
 * @param steps_ahead How many steps to predict
 * @return Prediction result
 */
Prediction ai_predict_linear(const float *data, int size, int steps_ahead);

/**
 * Predict next value using exponential smoothing
 *
 * @param data Historical data
 * @param size Number of historical points
 * @param alpha Smoothing factor (0-1)
 * @return Prediction result
 */
Prediction ai_predict_exponential(const float *data, int size, float alpha);

/**
 * NATURAL LANGUAGE QUERIES
 */

/**
 * Parse simple query string
 *
 * Example: "where x > 50 and value < 100"
 *
 * @param query Query string
 * @param tokens Output tokens
 * @param max_tokens Maximum tokens
 * @return Number of tokens parsed
 */
int ai_parse_query(const char *query, QueryToken *tokens, int max_tokens);

/**
 * Evaluate query on data point
 *
 * @param tokens Query tokens
 * @param num_tokens Number of tokens
 * @param x Data point X
 * @param y Data point Y
 * @param value Data point value
 * @return true if query matches
 */
bool ai_eval_query(const QueryToken *tokens, int num_tokens,
                    float x, float y, float value);

/**
 * UTILITY FUNCTIONS
 */

/* Calculate mean of array */
float ai_mean(const float *data, int size);

/* Calculate standard deviation */
float ai_stddev(const float *data, int size);

/* Calculate median */
float ai_median(const float *data, int size);

/* Calculate moving average */
float ai_moving_average(const float *data, int size, int window);

#endif /* AI_H */
