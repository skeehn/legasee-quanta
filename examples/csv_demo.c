#include "../src/csv_loader.h"
#include "../src/sim.h"
#include "../src/render.h"
#include "../src/term.h"
#include "../src/error.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

/* Map CSV data to particle colors based on value column */
static uint32_t value_to_color(float value, float min_val, float max_val) {
    /* Normalize value to [0, 1] */
    float t = (value - min_val) / (max_val - min_val);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* Color gradient: Blue (low) -> Green (mid) -> Red (high) */
    uint8_t r, g, b;
    if (t < 0.5f) {
        /* Blue to Green */
        float local_t = t * 2.0f;
        r = 0;
        g = (uint8_t)(local_t * 255);
        b = (uint8_t)((1.0f - local_t) * 255);
    } else {
        /* Green to Red */
        float local_t = (t - 0.5f) * 2.0f;
        r = (uint8_t)(local_t * 255);
        g = (uint8_t)((1.0f - local_t) * 255);
        b = 0;
    }

    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

int main(int argc, char *argv[]) {
    const char *csv_file = "data/example.csv";

    if (argc > 1) {
        csv_file = argv[1];
    }

    printf("=== CSV Particle Visualization Demo ===\n");
    printf("Loading CSV file: %s\n\n", csv_file);

    /* Load CSV data */
    CSVData *csv = NULL;
    Error err = csv_load(csv_file, &csv);
    if (err.code != SUCCESS) {
        error_print(&err);
        return EXIT_FAILURE;
    }

    /* Print CSV info */
    csv_print_info(csv);
    printf("\nFirst 5 rows:\n");
    csv_print_data(csv, 5);
    printf("\n");

    /* Find required columns */
    int x_col = csv_find_column(csv, "x");
    int y_col = csv_find_column(csv, "y");
    int speed_col = csv_find_column(csv, "speed");
    int value_col = csv_find_column(csv, "value");

    if (x_col < 0 || y_col < 0) {
        fprintf(stderr, "Error: CSV must have 'x' and 'y' columns\n");
        csv_free(csv);
        return EXIT_FAILURE;
    }

    printf("Column mapping:\n");
    printf("  x: column %d\n", x_col);
    printf("  y: column %d\n", y_col);
    if (speed_col >= 0) printf("  speed: column %d\n", speed_col);
    if (value_col >= 0) printf("  value: column %d\n", value_col);
    printf("\n");

    /* Find min/max values for color mapping */
    float min_value = 0.0f, max_value = 100.0f;
    if (value_col >= 0 && csv->num_rows > 0) {
        min_value = max_value = csv->data[0][value_col];
        for (int i = 1; i < csv->num_rows; i++) {
            float val = csv->data[i][value_col];
            if (val < min_value) min_value = val;
            if (val > max_value) max_value = val;
        }
        printf("Value range: %.2f - %.2f\n", min_value, max_value);
    }

    /* Setup terminal */
    int width = 80, height = 40;
    if (term_get_size(&width, &height) != 0) {
        width = 80;
        height = 40;
    }

    printf("\nStarting visualization (terminal: %dx%d)\n", width, height);
    printf("Press 'q' to quit...\n\n");

    sleep(2);

    /* Initialize terminal */
    if (term_init_raw() != 0) {
        fprintf(stderr, "Failed to initialize terminal\n");
        csv_free(csv);
        return EXIT_FAILURE;
    }

    /* Create renderer */
    Renderer *renderer;
    err = renderer_create_with_error(width, height, &renderer);
    if (err.code != SUCCESS) {
        error_print(&err);
        term_restore();
        csv_free(csv);
        return EXIT_FAILURE;
    }

    /* Create simulation */
    Simulation *sim = sim_create(csv->num_rows + 100, width, height);
    if (!sim) {
        fprintf(stderr, "Failed to create simulation\n");
        renderer_destroy(renderer);
        term_restore();
        csv_free(csv);
        return EXIT_FAILURE;
    }

    /* Disable gravity for data visualization */
    sim_set_gravity(sim, 0.0f);

    /* Load CSV data as particles */
    for (int i = 0; i < csv->num_rows; i++) {
        float x = csv->data[i][x_col];
        float y = csv->data[i][y_col];

        /* Default velocity */
        float vx = 0.0f, vy = 0.0f;

        /* Use speed column if available */
        if (speed_col >= 0) {
            float speed = csv->data[i][speed_col];
            /* Random direction */
            float angle = (float)i / csv->num_rows * 6.28f;
            vx = speed * cosf(angle) * 0.1f;
            vy = speed * sinf(angle) * 0.1f;
        }

        sim_add_particle(sim, x, y, vx, vy);
    }

    printf("Loaded %d particles from CSV\n", csv->num_rows);

    /* Main visualization loop */
    int frames = 0;
    const float dt = 1.0f / 60.0f;

    term_clear_screen();

    while (1) {
        /* Check for quit */
        if (term_kbhit()) {
            int ch = term_getch();
            if (ch == 'q' || ch == 'Q') {
                break;
            }
        }

        /* Update simulation (gentle drift) */
        sim_step(sim, dt);

        /* Clear renderer */
        renderer_clear(renderer);

        /* Render particles */
        int particle_count = sim_get_particle_count(sim);
        for (int i = 0; i < particle_count && i < csv->num_rows; i++) {
            const Particle *p = sim_get_particle(sim, i);
            if (p) {
                int px = (int)roundf(p->x);
                int py = (int)roundf(p->y);

                if (px >= 0 && px < width && py >= 0 && py < height) {
                    /* Get color from value column */
                    uint32_t color = 0x00AAFF; /* Default blue */
                    if (value_col >= 0) {
                        float value = csv->data[i][value_col];
                        color = value_to_color(value, min_value, max_value);
                    }

                    /* Choose glyph based on speed */
                    float speed = sqrtf(p->vx * p->vx + p->vy * p->vy);
                    char glyph = speed < 1.0f ? '.' : (speed < 2.0f ? 'o' : 'O');

                    renderer_plot(renderer, px, py, glyph, color);
                }
            }
        }

        /* Draw title */
        char title[128];
        snprintf(title, sizeof(title), "CSV Visualization: %s (%d points)",
                csv_file, csv->num_rows);
        renderer_draw_text(renderer, 0, 0, title, 0xFFFFFF);

        /* Draw legend */
        char legend[128];
        snprintf(legend, sizeof(legend),
                "Blue=Low Value, Green=Mid, Red=High | Frame: %d | Press 'q' to quit",
                frames);
        renderer_draw_text(renderer, 0, height - 1, legend, 0xAAAAAA);

        /* Flush to screen */
        renderer_flush(renderer);

        frames++;

        /* Frame rate limit */
        usleep(16667); /* ~60 FPS */
    }

    /* Cleanup */
    sim_destroy(sim);
    renderer_destroy(renderer);
    term_restore();
    csv_free(csv);

    printf("\nVisualization complete. Rendered %d frames.\n", frames);

    return EXIT_SUCCESS;
}
