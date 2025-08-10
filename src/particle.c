#include "particle.h"
#include <math.h>

/* Get particle speed */
float particle_get_speed(const Particle *p) {
    return sqrtf(p->vx * p->vx + p->vy * p->vy);
}

/* Initialize particle */
void particle_init(Particle *p, float x, float y, float vx, float vy) {
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
}

/* Reset particle */
void particle_reset(Particle *p) {
    p->x = 0.0f;
    p->y = 0.0f;
    p->vx = 0.0f;
    p->vy = 0.0f;
}
