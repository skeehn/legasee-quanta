#ifndef COLOR_H
#define COLOR_H

#include <stdio.h>
#include <math.h>

/* RGB color structure */
typedef struct {
    int r, g, b;
} RGB;

/* Set true color (24-bit RGB) */
static inline void color_set_rgb(int r, int g, int b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
}

/* Reset color to default */
static inline void color_reset(void) {
    printf("\033[0m");
}

/* Get color based on speed/energy */
static inline RGB color_from_speed(float speed) {
    RGB color;
    float normalized = fminf(speed / 20.0f, 1.0f);
    
    if (normalized < 0.5f) {
        /* Blue to Cyan (slow to medium) */
        float t = normalized * 2.0f;
        color.r = 0;
        color.g = (int)(128 * t);
        color.b = (int)(255 - 128 * t);
    } else {
        /* Cyan to Yellow to Red (medium to fast) */
        float t = (normalized - 0.5f) * 2.0f;
        color.r = (int)(255 * t);
        color.g = (int)(255 - 127 * t);
        color.b = (int)(128 * (1.0f - t));
    }
    
    return color;
}

/* Get color based on energy (velocity squared) */
static inline RGB color_from_energy(float energy) {
    RGB color;
    float normalized = fminf(energy / 400.0f, 1.0f);
    
    /* Heat map: Black -> Red -> Orange -> Yellow -> White */
    if (normalized < 0.33f) {
        float t = normalized * 3.0f;
        color.r = (int)(255 * t);
        color.g = 0;
        color.b = 0;
    } else if (normalized < 0.66f) {
        float t = (normalized - 0.33f) * 3.0f;
        color.r = 255;
        color.g = (int)(165 * t);
        color.b = 0;
    } else {
        float t = (normalized - 0.66f) * 3.0f;
        color.r = 255;
        color.g = (int)(165 + 90 * t);
        color.b = (int)(255 * t);
    }
    
    return color;
}

/* Apply RGB color */
static inline void color_apply(RGB color) {
    color_set_rgb(color.r, color.g, color.b);
}

/* Get faded color for trails */
static inline RGB color_fade(RGB color, float alpha) {
    RGB faded;
    faded.r = (int)(color.r * alpha);
    faded.g = (int)(color.g * alpha);
    faded.b = (int)(color.b * alpha);
    return faded;
}

#endif /* COLOR_H */

