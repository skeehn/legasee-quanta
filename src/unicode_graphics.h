#ifndef UNICODE_GRAPHICS_H
#define UNICODE_GRAPHICS_H

#include <math.h>

/* Unicode block characters for intensity */
static const char* BLOCKS[] = {
    " ",  /* 0/8 */
    "▁",  /* 1/8 */
    "▂",  /* 2/8 */
    "▃",  /* 3/8 */
    "▄",  /* 4/8 */
    "▅",  /* 5/8 */
    "▆",  /* 6/8 */
    "▇",  /* 7/8 */
    "█"   /* 8/8 */
};

/* Unicode dots and circles */
static const char* DOTS[] = {
    "·",  /* Light dot */
    "•",  /* Medium dot */
    "●",  /* Heavy dot */
    "⬤"   /* Large circle */
};

/* Directional arrows */
static const char* ARROWS[] = {
    "→",  /* Right */
    "↗",  /* Up-right */
    "↑",  /* Up */
    "↖",  /* Up-left */
    "←",  /* Left */
    "↙",  /* Down-left */
    "↓",  /* Down */
    "↘"   /* Down-right */
};

/* Get block character based on intensity (0.0 to 1.0) */
static inline const char* get_block_char(float intensity) {
    int index = (int)(intensity * 8.0f);
    if (index < 0) index = 0;
    if (index > 8) index = 8;
    return BLOCKS[index];
}

/* Get dot character based on size/intensity */
static inline const char* get_dot_char(float size) {
    if (size < 0.25f) return DOTS[0];
    if (size < 0.5f) return DOTS[1];
    if (size < 0.75f) return DOTS[2];
    return DOTS[3];
}

/* Get arrow character based on velocity direction */
static inline const char* get_arrow_char(float vx, float vy) {
    if (vx == 0.0f && vy == 0.0f) return DOTS[1];
    
    float angle = atan2f(vy, vx);
    /* Convert to 0-7 index (8 directions) */
    int index = (int)(((angle + M_PI) / (2.0f * M_PI)) * 8.0f + 0.5f) % 8;
    return ARROWS[index];
}

/* Get particle glyph based on speed and direction */
static inline const char* get_particle_glyph(float speed, float vx, float vy) {
    if (speed < 2.0f) return DOTS[0];      /* Very slow - light dot */
    if (speed < 5.0f) return DOTS[1];      /* Slow - medium dot */
    if (speed < 10.0f) return DOTS[2];     /* Medium - heavy dot */
    if (speed < 15.0f) return DOTS[3];     /* Fast - large circle */
    return get_arrow_char(vx, vy);         /* Very fast - directional arrow */
}

/* Get trail glyph (faded based on age) */
static inline const char* get_trail_glyph(int age, int max_age) {
    float intensity = 1.0f - ((float)age / (float)max_age);
    if (intensity < 0.3f) return DOTS[0];
    if (intensity < 0.6f) return DOTS[1];
    return DOTS[2];
}

#endif /* UNICODE_GRAPHICS_H */

