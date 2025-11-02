#include "../src/data_source.h"
#include "../src/csv_datasource.h"
#include "../src/json_datasource.h"
#include "../src/sim.h"
#include "../src/render.h"
#include "../src/term.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

/* Map value to color gradient */
static uint32_t value_to_color(float value, float min_val, float max_val) {
    float t = (value - min_val) / (max_val - min_val);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    uint8_t r, g, b;
    if (t < 0.5f) {
        float local_t = t * 2.0f;
        r = 0;
        g = (uint8_t)(local_t * 255);
        b = (uint8_t)((1.0f - local_t) * 255);
    } else {
        float local_t = (t - 0.5f) * 2.0f;
        r = (uint8_t)(local_t * 255);
        g = (uint8_t)((1.0f - local_t) * 255);
        b = 0;
    }

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* Detect file type from extension */
static const char* detect_file_type(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot) return NULL;

    if (strcmp(dot, ".csv") == 0) return "csv";
    if (strcmp(dot, ".json") == 0) return "json";

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <data_file.csv|data_file.json>\n", argv[0]);
        printf("\nSupported formats:\n");
        printf("  CSV:  Comma-separated values\n");
        printf("  JSON: Array of objects [{\"x\":1,\"y\":2,...}]\n");
        return EXIT_FAILURE;
    }

    const char *filename = argv[1];
    const char *file_type = detect_file_type(filename);

    if (!file_type) {
        fprintf(stderr, "Error: Unknown file type. Use .csv or .json\n");
        return EXIT_FAILURE;
    }

    printf("=== Unified Data Visualization Demo ===\n");
    printf("File: %s\n", filename);
    printf("Type: %s\n", file_type);
    printf("\n");

    /* Register plugins */
    csv_datasource_register();
    json_datasource_register();

    printf("Available data source plugins:\n");
    datasource_list_plugins();
    printf("\n");

    /* Create data source */
    DataSource *source = datasource_create(file_type);
    if (!source) {
        fprintf(stderr, "Error: Failed to create data source for type '%s'\n", file_type);
        return EXIT_FAILURE;
    }

    /* Initialize and open */
    Error err = datasource_init(source, filename);
    if (err.code != SUCCESS) {
        error_print(&err);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    err = datasource_open(source);
    if (err.code != SUCCESS) {
        error_print(&err);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    /* Get schema */
    DataSchema *schema;
    err = datasource_get_schema(source, &schema);
    if (err.code != SUCCESS) {
        error_print(&err);
        datasource_close(source);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    printf("Data Schema:\n");
    printf("  Columns: %d\n", schema->num_columns);
    for (int i = 0; i < schema->num_columns; i++) {
        printf("    %d: %s\n", i, schema->columns[i].name);
    }
    printf("\n");

    /* Find required columns */
    int x_col = schema_find_column(schema, "x");
    int y_col = schema_find_column(schema, "y");
    int speed_col = schema_find_column(schema, "speed");
    int value_col = schema_find_column(schema, "value");

    if (x_col < 0 || y_col < 0) {
        fprintf(stderr, "Error: Data must have 'x' and 'y' columns\n");
        schema_destroy(schema);
        datasource_close(source);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    /* Load all records and find min/max for visualization */
    typedef struct {
        float x, y, speed, value;
    } VisualizationRecord;

    VisualizationRecord *viz_records = malloc(1000 * sizeof(VisualizationRecord));
    int num_records = 0;
    float min_value = 0.0f, max_value = 100.0f;
    bool first_value = true;

    while (datasource_has_next(source) && num_records < 1000) {
        DataRecord *record;
        err = datasource_read_next(source, &record);
        if (err.code != SUCCESS) break;

        viz_records[num_records].x = record_get_float(record, x_col);
        viz_records[num_records].y = record_get_float(record, y_col);
        viz_records[num_records].speed = (speed_col >= 0) ?
            record_get_float(record, speed_col) : 5.0f;
        viz_records[num_records].value = (value_col >= 0) ?
            record_get_float(record, value_col) : 50.0f;

        if (value_col >= 0) {
            float val = viz_records[num_records].value;
            if (first_value) {
                min_value = max_value = val;
                first_value = false;
            } else {
                if (val < min_value) min_value = val;
                if (val > max_value) max_value = val;
            }
        }

        record_destroy(record);
        num_records++;
    }

    printf("Loaded %d records\n", num_records);
    if (value_col >= 0) {
        printf("Value range: %.2f - %.2f\n", min_value, max_value);
    }
    printf("\n");

    /* Setup visualization */
    int width = 80, height = 40;
    term_get_size(&width, &height);

    printf("Starting visualization (terminal: %dx%d)\n", width, height);
    printf("Press 'q' to quit...\n\n");
    sleep(2);

    if (term_init_raw() != 0) {
        fprintf(stderr, "Failed to initialize terminal\n");
        free(viz_records);
        schema_destroy(schema);
        datasource_close(source);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    Renderer *renderer;
    err = renderer_create_with_error(width, height, &renderer);
    if (err.code != SUCCESS) {
        error_print(&err);
        term_restore();
        free(viz_records);
        schema_destroy(schema);
        datasource_close(source);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    Simulation *sim = sim_create(num_records + 100, width, height);
    if (!sim) {
        renderer_destroy(renderer);
        term_restore();
        free(viz_records);
        schema_destroy(schema);
        datasource_close(source);
        datasource_destroy(source);
        return EXIT_FAILURE;
    }

    sim_set_gravity(sim, 5.0f);  /* Gentle gravity */

    /* Load data as particles */
    for (int i = 0; i < num_records; i++) {
        float angle = (float)i / num_records * 6.28f;
        float vx = viz_records[i].speed * cosf(angle) * 0.05f;
        float vy = viz_records[i].speed * sinf(angle) * 0.05f;
        sim_add_particle(sim, viz_records[i].x, viz_records[i].y, vx, vy);
    }

    /* Main loop */
    int frames = 0;
    const float dt = 1.0f / 60.0f;
    term_clear_screen();

    while (1) {
        if (term_kbhit()) {
            int ch = term_getch();
            if (ch == 'q' || ch == 'Q') break;
        }

        sim_step(sim, dt);
        renderer_clear(renderer);

        /* Render particles */
        int particle_count = sim_get_particle_count(sim);
        for (int i = 0; i < particle_count && i < num_records; i++) {
            const Particle *p = sim_get_particle(sim, i);
            if (p) {
                int px = (int)roundf(p->x);
                int py = (int)roundf(p->y);

                if (px >= 0 && px < width && py >= 0 && py < height) {
                    uint32_t color = (value_col >= 0) ?
                        value_to_color(viz_records[i].value, min_value, max_value) :
                        0x00AAFF;

                    float speed = sqrtf(p->vx * p->vx + p->vy * p->vy);
                    char glyph = speed < 1.0f ? '.' : (speed < 2.0f ? 'o' : 'O');

                    renderer_plot(renderer, px, py, glyph, color);
                }
            }
        }

        /* HUD */
        char title[128];
        snprintf(title, sizeof(title), "Data Viz: %s (%s, %d records)",
                filename, file_type, num_records);
        renderer_draw_text(renderer, 0, 0, title, 0xFFFFFF);

        char legend[128];
        snprintf(legend, sizeof(legend),
                "Blue=Low, Green=Mid, Red=High | Frame: %d | 'q'=Quit", frames);
        renderer_draw_text(renderer, 0, height - 1, legend, 0xAAAAAA);

        renderer_flush(renderer);
        frames++;
        usleep(16667);  /* 60 FPS */
    }

    /* Cleanup */
    sim_destroy(sim);
    renderer_destroy(renderer);
    term_restore();
    free(viz_records);
    schema_destroy(schema);
    datasource_close(source);
    datasource_destroy(source);

    printf("\nVisualization complete. Rendered %d frames.\n", frames);

    return EXIT_SUCCESS;
}
