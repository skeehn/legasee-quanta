/**
 * AI Features Demo (Week 4)
 *
 * Demonstrates:
 * - Anomaly Detection (Z-score and moving average)
 * - K-means Clustering
 * - Time Series Prediction
 * - Natural Language Queries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "../src/ai.h"
#include "../src/data_source.h"
#include "../src/csv_datasource.h"

#define MAX_DATA_POINTS 1000

/* Generate sample time series data with anomalies */
void generate_sample_data(float *data, int size) {
    srand((unsigned)time(NULL));

    for (int i = 0; i < size; i++) {
        /* Base trend: sinusoidal + linear growth */
        float base = 50.0f + i * 0.1f + 20.0f * sinf(i * 0.1f);

        /* Add small random noise */
        float noise = ((float)rand() / RAND_MAX - 0.5f) * 5.0f;

        /* Inject anomalies at specific points */
        if (i == size / 4 || i == size / 2 || i == 3 * size / 4) {
            noise += ((float)rand() / RAND_MAX - 0.5f) * 50.0f;  /* Large anomaly */
        }

        data[i] = base + noise;
    }
}

/* Print separator */
void print_separator(void) {
    printf("════════════════════════════════════════════════════════\n");
}

/* Demo 1: Anomaly Detection */
void demo_anomaly_detection(void) {
    printf("\n");
    print_separator();
    printf("1. ANOMALY DETECTION\n");
    print_separator();

    int size = 100;
    float *data = malloc(sizeof(float) * size);
    generate_sample_data(data, size);

    /* Z-score based detection */
    AnomalyResult results[20];
    int num_anomalies = ai_detect_anomalies_zscore(data, size, 2.5f, results, 20);

    printf("\nZ-Score Anomaly Detection (threshold=2.5):\n");
    printf("Found %d anomalies:\n\n", num_anomalies);

    for (int i = 0; i < num_anomalies && i < 5; i++) {
        printf("  Index %3d: Value=%.2f, Expected=%.2f, Z-score=%.2f\n",
               results[i].index, results[i].value,
               results[i].expected, results[i].deviation);
    }

    if (num_anomalies > 5) {
        printf("  ... and %d more anomalies\n", num_anomalies - 5);
    }

    /* Moving average based detection */
    num_anomalies = ai_detect_anomalies_moving_avg(data, size, 10, 15.0f,
                                                    results, 20);

    printf("\nMoving Average Anomaly Detection (window=10, threshold=15):\n");
    printf("Found %d anomalies:\n\n", num_anomalies);

    for (int i = 0; i < num_anomalies && i < 5; i++) {
        printf("  Index %3d: Value=%.2f, MA=%.2f, Deviation=%.2f\n",
               results[i].index, results[i].value,
               results[i].expected, results[i].deviation);
    }

    free(data);
}

/* Demo 2: K-Means Clustering */
void demo_clustering(void) {
    printf("\n");
    print_separator();
    printf("2. K-MEANS CLUSTERING\n");
    print_separator();

    int num_points = 200;
    float *x = malloc(sizeof(float) * num_points);
    float *y = malloc(sizeof(float) * num_points);
    float *values = malloc(sizeof(float) * num_points);

    /* Generate 3 clusters of points */
    for (int i = 0; i < num_points; i++) {
        int cluster = i / (num_points / 3);
        float base_x = (cluster % 2) * 60.0f + 20.0f;
        float base_y = (cluster / 2) * 30.0f + 10.0f;

        x[i] = base_x + ((float)rand() / RAND_MAX - 0.5f) * 20.0f;
        y[i] = base_y + ((float)rand() / RAND_MAX - 0.5f) * 15.0f;
        values[i] = 50.0f + ((float)rand() / RAND_MAX) * 50.0f;
    }

    /* Perform clustering */
    ClusteringResult result;
    Error err = ai_kmeans_cluster(x, y, values, num_points, 3, 50, &result);

    if (err.code == SUCCESS) {
        printf("\nClustered %d points into %d clusters:\n\n", num_points, result.num_clusters);

        for (int i = 0; i < result.num_clusters; i++) {
            printf("Cluster %d:\n", i);
            printf("  Centroid: (%.1f, %.1f)\n",
                   result.clusters[i].centroid_x,
                   result.clusters[i].centroid_y);
            printf("  Points:   %d\n", result.clusters[i].count);
            printf("  Avg Value: %.1f\n", result.clusters[i].avg_value);
            printf("\n");
        }

        printf("Total Inertia: %.2f\n", result.inertia);

        ai_clustering_result_free(&result);
    } else {
        printf("Clustering failed: %s\n", err.message);
    }

    free(x);
    free(y);
    free(values);
}

/* Demo 3: Time Series Prediction */
void demo_prediction(void) {
    printf("\n");
    print_separator();
    printf("3. TIME SERIES PREDICTION\n");
    print_separator();

    int size = 50;
    float *data = malloc(sizeof(float) * size);

    /* Generate simple time series */
    for (int i = 0; i < size; i++) {
        data[i] = 100.0f + i * 2.0f + ((float)rand() / RAND_MAX - 0.5f) * 10.0f;
    }

    printf("\nHistorical Data (last 10 points):\n");
    for (int i = size - 10; i < size; i++) {
        printf("  t=%2d: %.2f\n", i, data[i]);
    }

    /* Linear prediction */
    Prediction pred_linear = ai_predict_linear(data, size, 5);
    printf("\nLinear Regression Prediction (t+5):\n");
    printf("  Value:      %.2f\n", pred_linear.value);
    printf("  Confidence: %.2f\n", pred_linear.confidence);
    printf("  Range:      [%.2f, %.2f]\n",
           pred_linear.lower_bound, pred_linear.upper_bound);

    /* Exponential smoothing */
    Prediction pred_exp = ai_predict_exponential(data, size, 0.3f);
    printf("\nExponential Smoothing Prediction (alpha=0.3):\n");
    printf("  Value:      %.2f\n", pred_exp.value);
    printf("  Confidence: %.2f\n", pred_exp.confidence);
    printf("  Range:      [%.2f, %.2f]\n",
           pred_exp.lower_bound, pred_exp.upper_bound);

    free(data);
}

/* Demo 4: Natural Language Queries */
void demo_queries(void) {
    printf("\n");
    print_separator();
    printf("4. NATURAL LANGUAGE QUERIES\n");
    print_separator();

    const char *queries[] = {
        "where x > 50",
        "where x > 50 and y < 30",
        "where value >= 75",
        "where x > 20 and value < 80"
    };

    int num_queries = sizeof(queries) / sizeof(queries[0]);

    /* Sample data points */
    struct {
        float x, y, value;
    } points[] = {
        {10, 20, 45},
        {60, 25, 82},
        {55, 35, 60},
        {30, 15, 90},
        {70, 10, 50}
    };

    int num_points = sizeof(points) / sizeof(points[0]);

    for (int q = 0; q < num_queries; q++) {
        printf("\nQuery: \"%s\"\n", queries[q]);

        /* Parse query */
        QueryToken tokens[AI_MAX_QUERY_TOKENS];
        int num_tokens = ai_parse_query(queries[q], tokens, AI_MAX_QUERY_TOKENS);

        printf("Tokens: ");
        for (int i = 0; i < num_tokens; i++) {
            printf("[%s] ", tokens[i].text);
        }
        printf("\n");

        /* Evaluate on sample points */
        printf("Matching points:\n");
        int matches = 0;

        for (int i = 0; i < num_points; i++) {
            if (ai_eval_query(tokens, num_tokens,
                            points[i].x, points[i].y, points[i].value)) {
                printf("  Point %d: (x=%.1f, y=%.1f, value=%.1f)\n",
                       i, points[i].x, points[i].y, points[i].value);
                matches++;
            }
        }

        printf("Total matches: %d/%d\n", matches, num_points);
    }
}

/* Demo 5: Utility Functions */
void demo_utilities(void) {
    printf("\n");
    print_separator();
    printf("5. STATISTICAL UTILITIES\n");
    print_separator();

    float data[] = {10, 20, 15, 30, 25, 18, 22, 28, 16, 24};
    int size = sizeof(data) / sizeof(data[0]);

    printf("\nData: ");
    for (int i = 0; i < size; i++) {
        printf("%.0f ", data[i]);
    }
    printf("\n\n");

    printf("Mean:           %.2f\n", ai_mean(data, size));
    printf("Median:         %.2f\n", ai_median(data, size));
    printf("Std Deviation:  %.2f\n", ai_stddev(data, size));
    printf("Moving Avg (5): %.2f\n", ai_moving_average(data, size, 5));
}

int main(void) {
    printf("\n");
    print_separator();
    printf("     ASCII PARTICLE SIMULATOR - AI FEATURES DEMO     \n");
    printf("                    (Week 4)                          \n");
    print_separator();

    srand((unsigned)time(NULL));

    demo_anomaly_detection();
    demo_clustering();
    demo_prediction();
    demo_queries();
    demo_utilities();

    printf("\n");
    print_separator();
    printf("              ALL AI FEATURES DEMONSTRATED            \n");
    print_separator();
    printf("\n");

    printf("Summary:\n");
    printf("  ✓ Anomaly Detection: Z-score and Moving Average methods\n");
    printf("  ✓ K-Means Clustering: Pattern recognition in 2D space\n");
    printf("  ✓ Time Series Prediction: Linear and Exponential methods\n");
    printf("  ✓ Natural Language Queries: Simple data filtering\n");
    printf("  ✓ Statistical Utilities: Mean, Median, StdDev, Moving Avg\n");
    printf("\n");

    printf("Week 4 AI Features: COMPLETE ✓\n\n");

    return 0;
}
