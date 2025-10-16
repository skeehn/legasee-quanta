#ifndef PARTICLE_ENHANCED_H
#define PARTICLE_ENHANCED_H

#include "particle.h"

/* Enhanced particle with trail history */
#define TRAIL_LENGTH 5

typedef struct {
    float x, y;           /* Current position */
    float vx, vy;         /* Velocity */
    float trail_x[TRAIL_LENGTH];  /* Trail positions */
    float trail_y[TRAIL_LENGTH];
    int trail_index;      /* Current trail write position */
    int trail_count;      /* Number of valid trail points */
    float energy;         /* Particle energy for color */
    int type;             /* Particle type (0=normal, 1=heavy, 2=light) */
} ParticleEnhanced;

/* Initialize enhanced particle */
static inline void particle_enhanced_init(ParticleEnhanced *p, float x, float y, float vx, float vy) {
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->trail_index = 0;
    p->trail_count = 0;
    p->energy = vx * vx + vy * vy;
    p->type = 0;
    
    for (int i = 0; i < TRAIL_LENGTH; i++) {
        p->trail_x[i] = x;
        p->trail_y[i] = y;
    }
}

/* Update trail history */
static inline void particle_enhanced_update_trail(ParticleEnhanced *p) {
    p->trail_x[p->trail_index] = p->x;
    p->trail_y[p->trail_index] = p->y;
    p->trail_index = (p->trail_index + 1) % TRAIL_LENGTH;
    if (p->trail_count < TRAIL_LENGTH) {
        p->trail_count++;
    }
    p->energy = p->vx * p->vx + p->vy * p->vy;
}

/* Convert to basic particle for compatibility */
static inline Particle particle_enhanced_to_basic(const ParticleEnhanced *p) {
    Particle basic = {p->x, p->y, p->vx, p->vy};
    return basic;
}

#endif /* PARTICLE_ENHANCED_H */

