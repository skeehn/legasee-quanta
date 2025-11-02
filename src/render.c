#include "render.h"
#include "term.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Note: Old renderer_create function removed - use renderer_create_with_error instead */

/* Destroy renderer and free all memory */
void renderer_destroy(Renderer *renderer) {
    if (renderer) {
        free(renderer->glyphs);
        free(renderer->colors);
        free(renderer->row_buffer);
        free(renderer);
    }
}

/* Clear the renderer with default values */
void renderer_clear(Renderer *renderer) {
    if (!renderer) return;
    
    /* Fill with spaces and default color */
    memset(renderer->glyphs, ' ', renderer->width * renderer->height);
    for (int i = 0; i < renderer->width * renderer->height; i++) {
        renderer->colors[i] = 0x202020; /* Dark gray */
    }
}

/* Plot a character at specified position with color */
void renderer_plot(Renderer *renderer, int x, int y, char glyph, uint32_t color) {
    if (!renderer || x < 0 || x >= renderer->width || y < 0 || y >= renderer->height) {
        return; /* Out of bounds */
    }
    
    int index = y * renderer->width + x;
    renderer->glyphs[index] = glyph;
    renderer->colors[index] = color;
}

/* Draw text at specified position */
void renderer_draw_text(Renderer *renderer, int x, int y, const char *text, uint32_t color) {
    if (!renderer || !text || y < 0 || y >= renderer->height) {
        return;
    }
    
    int len = strlen(text);
    for (int i = 0; i < len && (x + i) < renderer->width; i++) {
        if ((x + i) >= 0) {
            renderer_plot(renderer, x + i, y, text[i], color);
        }
    }
}

/* Get renderer dimensions */
void renderer_get_size(const Renderer *renderer, int *width, int *height) {
    if (renderer) {
        if (width) *width = renderer->width;
        if (height) *height = renderer->height;
    }
}

/* Convert RGB values to packed color */
uint32_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

/* Convert packed color to RGB values */
void color_to_rgb(uint32_t color, uint8_t *r, uint8_t *g, uint8_t *b) {
    if (r) *r = (color >> 16) & 0xFF;
    if (g) *g = (color >> 8) & 0xFF;
    if (b) *b = color & 0xFF;
}

/* Optimized flush function with row buffering and color caching */
void renderer_flush(Renderer *renderer) {
    if (!renderer) return;
    
    /* Move cursor to home position */
    term_home();
    
    /* Build and output each row */
    for (int y = 0; y < renderer->height; y++) {
        char *buffer = renderer->row_buffer;
        int buffer_pos = 0;
        uint32_t last_color = 0xFFFFFFFF; /* Invalid color to force first color */
        
        /* Build the complete row in memory */
        for (int x = 0; x < renderer->width; x++) {
            int index = y * renderer->width + x;
            char glyph = renderer->glyphs[index];
            uint32_t color = renderer->colors[index];
            
            /* Only add color escape if it changed (optimization) */
            if (color != last_color) {
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;

                buffer_pos += snprintf(buffer + buffer_pos,
                                      renderer->row_buffer_size - buffer_pos,
                                      "\033[38;2;%d;%d;%dm", r, g, b);
                last_color = color;
            }
            
            /* Add glyph */
            buffer[buffer_pos++] = glyph;
        }
        
        /* Add newline and write the entire row */
        buffer[buffer_pos++] = '\n';
        
        /* Single write per row for maximum efficiency */
        fwrite(buffer, 1, buffer_pos, stdout);
    }
    
    fflush(stdout);
}

/* Create a color gradient for testing */
void renderer_test_gradient(Renderer *renderer) {
    if (!renderer) return;
    
    for (int y = 0; y < renderer->height; y++) {
        for (int x = 0; x < renderer->width; x++) {
            /* Create a rainbow gradient */
            float hue = (float)x / renderer->width * 360.0f;
            float sat = 0.8f;
            float val = 0.8f;
            
            /* Convert HSV to RGB */
            float c = val * sat;
            float x_hsv = c * (1.0f - fabsf(fmodf(hue / 60.0f, 2.0f) - 1.0f));
            float m = val - c;
            
            float r, g, b;
            if (hue < 60) {
                r = c; g = x_hsv; b = 0;
            } else if (hue < 120) {
                r = x_hsv; g = c; b = 0;
            } else if (hue < 180) {
                r = 0; g = c; b = x_hsv;
            } else if (hue < 240) {
                r = 0; g = x_hsv; b = c;
            } else if (hue < 300) {
                r = x_hsv; g = 0; b = c;
            } else {
                r = c; g = 0; b = x_hsv;
            }
            
            uint8_t r8 = (uint8_t)((r + m) * 255);
            uint8_t g8 = (uint8_t)((g + m) * 255);
            uint8_t b8 = (uint8_t)((b + m) * 255);
            
            char glyph = " .:-=+*#%@"[(x + y) % 11];
            renderer_plot(renderer, x, y, glyph, rgb_to_color(r8, g8, b8));
        }
    }
}

/* ===== ERROR-AWARE RENDERER FUNCTIONS ===== */

/* Create a new renderer with error handling */
Error renderer_create_with_error(int width, int height, Renderer **renderer_out) {
    ERROR_CHECK(renderer_out != NULL, ERROR_NULL_POINTER, "Renderer output pointer cannot be NULL");
    ERROR_CHECK(width > 0, ERROR_INVALID_PARAMETER, "Width must be positive");
    ERROR_CHECK(height > 0, ERROR_INVALID_PARAMETER, "Height must be positive");
    ERROR_CHECK(width <= 1000, ERROR_INVALID_PARAMETER, "Width too large (max 1000)");
    ERROR_CHECK(height <= 1000, ERROR_INVALID_PARAMETER, "Height too large (max 1000)");

    /* Check for potential integer overflow in buffer allocation */
    size_t total_size = (size_t)width * (size_t)height;
    ERROR_CHECK(total_size <= SIZE_MAX / sizeof(uint32_t), ERROR_INVALID_PARAMETER,
               "Width × Height too large, would cause overflow");

    Renderer *renderer = error_malloc(sizeof(Renderer));
    if (!renderer) {
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate renderer structure");
    }
    
    renderer->width = width;
    renderer->height = height;
    
    /* Allocate glyph and color arrays */
    renderer->glyphs = error_malloc(width * height * sizeof(char));
    renderer->colors = error_malloc(width * height * sizeof(uint32_t));
    
    if (!renderer->glyphs || !renderer->colors) {
        renderer_destroy(renderer);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate renderer buffers");
    }
    
    /* Allocate row buffer for efficient output */
    /* Estimate max row size: width * (color_escape + glyph) + newline */
    size_t max_row_size = width * 32 + 2; /* Conservative estimate */
    renderer->row_buffer = error_malloc(max_row_size);
    renderer->row_buffer_size = max_row_size;

    if (!renderer->row_buffer) {
        renderer_destroy(renderer);
        return ERROR_CREATE(ERROR_MEMORY_ALLOCATION, "Failed to allocate row buffer");
    }
    
    /* Initialize with default values */
    renderer_clear(renderer);
    
    *renderer_out = renderer;
    return (Error){SUCCESS};
}

/* Plot a character with error handling */
Error renderer_plot_with_error(Renderer *renderer, int x, int y, char glyph, uint32_t color) {
    ERROR_CHECK(renderer != NULL, ERROR_NULL_POINTER, "Renderer cannot be NULL");
    ERROR_CHECK(x >= 0 && x < renderer->width, ERROR_OUT_OF_RANGE, "X coordinate out of bounds");
    ERROR_CHECK(y >= 0 && y < renderer->height, ERROR_OUT_OF_RANGE, "Y coordinate out of bounds");
    
    int index = y * renderer->width + x;
    renderer->glyphs[index] = glyph;
    renderer->colors[index] = color;
    
    return (Error){SUCCESS};
}

/* Draw text with error handling */
Error renderer_draw_text_with_error(Renderer *renderer, int x, int y, const char *text, uint32_t color) {
    ERROR_CHECK(renderer != NULL, ERROR_NULL_POINTER, "Renderer cannot be NULL");
    ERROR_CHECK(text != NULL, ERROR_NULL_POINTER, "Text cannot be NULL");
    ERROR_CHECK(y >= 0 && y < renderer->height, ERROR_OUT_OF_RANGE, "Y coordinate out of bounds");
    
    int len = strlen(text);
    for (int i = 0; i < len && (x + i) < renderer->width; i++) {
        if ((x + i) >= 0) {
            Error err = renderer_plot_with_error(renderer, x + i, y, text[i], color);
            if (err.code != SUCCESS) {
                return err;
            }
        }
    }
    
    return (Error){SUCCESS};
}

/* Get renderer size with error handling */
Error renderer_get_size_with_error(const Renderer *renderer, int *width_out, int *height_out) {
    ERROR_CHECK(renderer != NULL, ERROR_NULL_POINTER, "Renderer cannot be NULL");
    ERROR_CHECK(width_out != NULL, ERROR_NULL_POINTER, "Width output pointer cannot be NULL");
    ERROR_CHECK(height_out != NULL, ERROR_NULL_POINTER, "Height output pointer cannot be NULL");
    
    *width_out = renderer->width;
    *height_out = renderer->height;
    
    return (Error){SUCCESS};
}

/* Flush renderer with error handling */
Error renderer_flush_with_error(Renderer *renderer) {
    ERROR_CHECK(renderer != NULL, ERROR_NULL_POINTER, "Renderer cannot be NULL");
    
    /* Move cursor to home position */
    term_home();
    
    /* Build and output each row */
    for (int y = 0; y < renderer->height; y++) {
        char *buffer = renderer->row_buffer;
        int buffer_pos = 0;
        uint32_t last_color = 0xFFFFFFFF; /* Invalid color to force first color */
        
        /* Build the complete row in memory */
        for (int x = 0; x < renderer->width; x++) {
            int index = y * renderer->width + x;
            char glyph = renderer->glyphs[index];
            uint32_t color = renderer->colors[index];
            
            /* Only add color escape if it changed (optimization) */
            if (color != last_color) {
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t g = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;

                buffer_pos += snprintf(buffer + buffer_pos,
                                      renderer->row_buffer_size - buffer_pos,
                                      "\033[38;2;%d;%d;%dm", r, g, b);
                last_color = color;
            }
            
            /* Add glyph */
            buffer[buffer_pos++] = glyph;
        }
        
        /* Add newline and write the entire row */
        buffer[buffer_pos++] = '\n';
        
        /* Single write per row for maximum efficiency */
        if (fwrite(buffer, 1, buffer_pos, stdout) != (size_t)buffer_pos) {
            return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to write to stdout");
        }
    }
    
    if (fflush(stdout) != 0) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to flush stdout");
    }
    
    return (Error){SUCCESS};
} 
