#include "spatial_grid.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Helper: Get cell at row, col */
static inline GridCell* get_cell(SpatialGrid *grid, int row, int col) {
    if (row < 0 || row >= grid->rows || col < 0 || col >= grid->cols) {
        return NULL;
    }
    return &grid->cells[row * grid->cols + col];
}

/* Helper: Initialize a grid cell */
static Error grid_cell_init(GridCell *cell) {
    cell->capacity = GRID_MAX_PARTICLES_PER_CELL;
    cell->particles = malloc(sizeof(Particle*) * cell->capacity);
    if (!cell->particles) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate grid cell");
    }
    cell->count = 0;
    return (Error){SUCCESS};
}

/* Helper: Free a grid cell */
static void grid_cell_free(GridCell *cell) {
    if (cell->particles) {
        free(cell->particles);
        cell->particles = NULL;
    }
    cell->count = 0;
    cell->capacity = 0;
}

/* Helper: Clear a grid cell */
static void grid_cell_clear(GridCell *cell) {
    cell->count = 0;
}

/* Helper: Add particle to cell */
static Error grid_cell_add(GridCell *cell, Particle *particle) {
    if (cell->count >= cell->capacity) {
        /* Expand capacity */
        int new_capacity = cell->capacity * 2;
        Particle **new_particles = realloc(cell->particles,
                                           sizeof(Particle*) * new_capacity);
        if (!new_particles) {
            return ERROR_CREATE(ERROR_MEMORY_ALLOCATION,
                              "Failed to expand grid cell");
        }
        cell->particles = new_particles;
        cell->capacity = new_capacity;
    }

    cell->particles[cell->count++] = particle;
    return (Error){SUCCESS};
}

/* Create spatial grid */
SpatialGrid* spatial_grid_create(int world_width, int world_height, float cell_size) {
    if (world_width <= 0 || world_height <= 0 || cell_size <= 0) {
        return NULL;
    }

    SpatialGrid *grid = malloc(sizeof(SpatialGrid));
    if (!grid) return NULL;

    /* Calculate grid dimensions */
    grid->cols = (int)ceilf(world_width / cell_size);
    grid->rows = (int)ceilf(world_height / cell_size);

    /* Ensure minimum grid size */
    if (grid->cols < 2) grid->cols = 2;
    if (grid->rows < 2) grid->rows = 2;

    /* Calculate actual cell size */
    grid->cell_width = (float)world_width / grid->cols;
    grid->cell_height = (float)world_height / grid->rows;
    grid->world_width = world_width;
    grid->world_height = world_height;
    grid->total_particles = 0;

    /* Allocate cells */
    int total_cells = grid->rows * grid->cols;
    grid->cells = calloc(total_cells, sizeof(GridCell));
    if (!grid->cells) {
        free(grid);
        return NULL;
    }

    /* Initialize each cell */
    for (int i = 0; i < total_cells; i++) {
        Error err = grid_cell_init(&grid->cells[i]);
        if (err.code != SUCCESS) {
            /* Cleanup on error */
            for (int j = 0; j < i; j++) {
                grid_cell_free(&grid->cells[j]);
            }
            free(grid->cells);
            free(grid);
            return NULL;
        }
    }

    return grid;
}

/* Destroy spatial grid */
void spatial_grid_destroy(SpatialGrid *grid) {
    if (!grid) return;

    if (grid->cells) {
        int total_cells = grid->rows * grid->cols;
        for (int i = 0; i < total_cells; i++) {
            grid_cell_free(&grid->cells[i]);
        }
        free(grid->cells);
    }

    free(grid);
}

/* Clear all particles */
void spatial_grid_clear(SpatialGrid *grid) {
    if (!grid) return;

    int total_cells = grid->rows * grid->cols;
    for (int i = 0; i < total_cells; i++) {
        grid_cell_clear(&grid->cells[i]);
    }
    grid->total_particles = 0;
}

/* Convert world coordinates to cell indices */
bool spatial_grid_world_to_cell(SpatialGrid *grid, float x, float y,
                                int *col_out, int *row_out) {
    if (!grid) return false;

    int col = (int)(x / grid->cell_width);
    int row = (int)(y / grid->cell_height);

    /* Clamp to grid bounds */
    if (col < 0) col = 0;
    if (col >= grid->cols) col = grid->cols - 1;
    if (row < 0) row = 0;
    if (row >= grid->rows) row = grid->rows - 1;

    if (col_out) *col_out = col;
    if (row_out) *row_out = row;

    /* Return true if within original bounds */
    return (x >= 0 && x < grid->world_width &&
            y >= 0 && y < grid->world_height);
}

/* Insert particle into grid */
Error spatial_grid_insert(SpatialGrid *grid, Particle *particle) {
    ERROR_CHECK_NULL(grid, "Spatial grid");
    ERROR_CHECK_NULL(particle, "Particle");

    int col, row;
    spatial_grid_world_to_cell(grid, particle->x, particle->y, &col, &row);

    GridCell *cell = get_cell(grid, row, col);
    if (!cell) {
        return ERROR_CREATE(ERROR_OUT_OF_RANGE, "Cell out of bounds");
    }

    Error err = grid_cell_add(cell, particle);
    if (err.code == SUCCESS) {
        grid->total_particles++;
    }

    return err;
}

/* Get particles in specific cell */
int spatial_grid_get_cell(SpatialGrid *grid, float x, float y,
                          Particle **particles_out, int max_particles) {
    if (!grid || !particles_out) return 0;

    int col, row;
    spatial_grid_world_to_cell(grid, x, y, &col, &row);

    GridCell *cell = get_cell(grid, row, col);
    if (!cell) return 0;

    int count = cell->count < max_particles ? cell->count : max_particles;
    memcpy(particles_out, cell->particles, count * sizeof(Particle*));

    return count;
}

/* Get particles in 3x3 neighborhood */
int spatial_grid_get_neighbors(SpatialGrid *grid, float x, float y,
                               Particle **particles_out, int max_particles) {
    if (!grid || !particles_out) return 0;

    int center_col, center_row;
    spatial_grid_world_to_cell(grid, x, y, &center_col, &center_row);

    int total = 0;

    /* Check 3x3 neighborhood */
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            int row = center_row + dr;
            int col = center_col + dc;

            GridCell *cell = get_cell(grid, row, col);
            if (!cell) continue;

            /* Add particles from this cell */
            for (int i = 0; i < cell->count && total < max_particles; i++) {
                particles_out[total++] = cell->particles[i];
            }

            if (total >= max_particles) {
                return total;
            }
        }
    }

    return total;
}

/* Query particles within radius */
int spatial_grid_query_radius(SpatialGrid *grid, float x, float y, float radius,
                              Particle **particles_out, int max_particles) {
    if (!grid || !particles_out) return 0;

    int center_col, center_row;
    spatial_grid_world_to_cell(grid, x, y, &center_col, &center_row);

    /* Calculate search range in cells */
    int cell_radius = (int)ceilf(radius / fminf(grid->cell_width, grid->cell_height));
    int total = 0;
    float radius_sq = radius * radius;

    /* Check cells within radius */
    for (int dr = -cell_radius; dr <= cell_radius; dr++) {
        for (int dc = -cell_radius; dc <= cell_radius; dc++) {
            int row = center_row + dr;
            int col = center_col + dc;

            GridCell *cell = get_cell(grid, row, col);
            if (!cell) continue;

            /* Check particles in cell */
            for (int i = 0; i < cell->count && total < max_particles; i++) {
                Particle *p = cell->particles[i];

                /* Distance check */
                float dx = p->x - x;
                float dy = p->y - y;
                float dist_sq = dx * dx + dy * dy;

                if (dist_sq <= radius_sq) {
                    particles_out[total++] = p;
                }
            }

            if (total >= max_particles) {
                return total;
            }
        }
    }

    return total;
}

/* Get grid statistics */
void spatial_grid_get_stats(SpatialGrid *grid, GridStats *stats) {
    if (!grid || !stats) return;

    memset(stats, 0, sizeof(GridStats));

    stats->total_cells = grid->rows * grid->cols;
    stats->min_particles_per_cell = INT32_MAX;
    stats->max_particles_per_cell = 0;
    stats->total_particles = grid->total_particles;

    int total_in_occupied = 0;

    for (int i = 0; i < stats->total_cells; i++) {
        GridCell *cell = &grid->cells[i];

        if (cell->count > 0) {
            stats->occupied_cells++;
            total_in_occupied += cell->count;

            if (cell->count < stats->min_particles_per_cell) {
                stats->min_particles_per_cell = cell->count;
            }
            if (cell->count > stats->max_particles_per_cell) {
                stats->max_particles_per_cell = cell->count;
            }
        }
    }

    stats->empty_cells = stats->total_cells - stats->occupied_cells;

    if (stats->occupied_cells > 0) {
        stats->avg_particles_per_cell = (float)total_in_occupied / stats->occupied_cells;
    }

    if (stats->min_particles_per_cell == INT32_MAX) {
        stats->min_particles_per_cell = 0;
    }
}
