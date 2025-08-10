#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

/* Renderer structure */
typedef struct {
    char *glyphs;
    uint32_t *colors;
    int width, height;
    char *row_buffer;
} Renderer;

/* Core renderer functions */
Renderer *renderer_create(int width, int height);
void renderer_destroy(Renderer *renderer);
void renderer_clear(Renderer *renderer);
void renderer_plot(Renderer *renderer, int x, int y, char glyph, uint32_t color);
void renderer_flush(Renderer *renderer);

/* Utility functions */
void renderer_draw_text(Renderer *renderer, int x, int y, const char *text, uint32_t color);
void renderer_get_size(const Renderer *renderer, int *width, int *height);

/* Color utilities */
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b);
void color_to_rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b);

/* Test functions */
void renderer_test_gradient(Renderer *renderer);

#endif /* RENDER_H */ 
