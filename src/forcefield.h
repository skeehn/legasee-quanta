#ifndef FORCEFIELD_H
#define FORCEFIELD_H

#include <math.h>

/* Force field types */
typedef enum {
    FIELD_ATTRACTOR,    /* Pulls particles in */
    FIELD_REPELLER,     /* Pushes particles away */
    FIELD_VORTEX,       /* Circular motion */
    FIELD_DIRECTIONAL   /* Constant direction */
} ForceFieldType;

/* Force field structure */
typedef struct {
    float x, y;              /* Position */
    float strength;          /* Field strength */
    float radius;            /* Effective radius */
    ForceFieldType type;     /* Field type */
    int active;              /* 1=active, 0=inactive */
} ForceField;

/* Maximum number of force fields */
#define MAX_FORCE_FIELDS 10

/* Force field manager */
typedef struct {
    ForceField fields[MAX_FORCE_FIELDS];
    int count;
} ForceFieldManager;

/* Initialize force field manager */
static inline void forcefield_init(ForceFieldManager *mgr) {
    mgr->count = 0;
    for (int i = 0; i < MAX_FORCE_FIELDS; i++) {
        mgr->fields[i].active = 0;
    }
}

/* Add a force field */
static inline int forcefield_add(ForceFieldManager *mgr, float x, float y, 
                                 float strength, float radius, ForceFieldType type) {
    if (mgr->count >= MAX_FORCE_FIELDS) return -1;
    
    ForceField *field = &mgr->fields[mgr->count];
    field->x = x;
    field->y = y;
    field->strength = strength;
    field->radius = radius;
    field->type = type;
    field->active = 1;
    
    mgr->count++;
    return mgr->count - 1;
}

/* Remove a force field */
static inline void forcefield_remove(ForceFieldManager *mgr, int index) {
    if (index < 0 || index >= mgr->count) return;
    
    mgr->fields[index].active = 0;
    
    /* Compact array */
    for (int i = index; i < mgr->count - 1; i++) {
        mgr->fields[i] = mgr->fields[i + 1];
    }
    mgr->count--;
}

/* Clear all force fields */
static inline void forcefield_clear(ForceFieldManager *mgr) {
    mgr->count = 0;
    for (int i = 0; i < MAX_FORCE_FIELDS; i++) {
        mgr->fields[i].active = 0;
    }
}

/* Apply force field to particle */
static inline void forcefield_apply(const ForceField *field, float *px, float *py, 
                                    float *vx, float *vy, float dt) {
    if (!field->active) return;
    
    float dx = field->x - *px;
    float dy = field->y - *py;
    float dist_sq = dx * dx + dy * dy;
    float dist = sqrtf(dist_sq);
    
    /* Check if within radius */
    if (dist > field->radius) return;
    
    float fx = 0.0f, fy = 0.0f;
    
    switch (field->type) {
        case FIELD_ATTRACTOR:
            /* Inverse square law attraction */
            if (dist > 0.1f) {
                float force = field->strength / (dist_sq + 1.0f);
                fx = (dx / dist) * force;
                fy = (dy / dist) * force;
            }
            break;
            
        case FIELD_REPELLER:
            /* Inverse square law repulsion */
            if (dist > 0.1f) {
                float force = field->strength / (dist_sq + 1.0f);
                fx = -(dx / dist) * force;
                fy = -(dy / dist) * force;
            }
            break;
            
        case FIELD_VORTEX:
            /* Tangential force for rotation */
            if (dist > 0.1f) {
                float force = field->strength / (dist + 1.0f);
                fx = -(dy / dist) * force;  /* Perpendicular to radius */
                fy = (dx / dist) * force;
            }
            break;
            
        case FIELD_DIRECTIONAL:
            /* Constant directional force */
            fx = field->strength * cosf(field->radius);  /* Use radius as angle */
            fy = field->strength * sinf(field->radius);
            break;
    }
    
    /* Apply force to velocity */
    *vx += fx * dt;
    *vy += fy * dt;
}

/* Apply all force fields to particle */
static inline void forcefield_apply_all(const ForceFieldManager *mgr, float *px, float *py,
                                        float *vx, float *vy, float dt) {
    for (int i = 0; i < mgr->count; i++) {
        if (mgr->fields[i].active) {
            forcefield_apply(&mgr->fields[i], px, py, vx, vy, dt);
        }
    }
}

/* Get force field at index */
static inline ForceField* forcefield_get(ForceFieldManager *mgr, int index) {
    if (index < 0 || index >= mgr->count) return NULL;
    return &mgr->fields[index];
}

#endif /* FORCEFIELD_H */

