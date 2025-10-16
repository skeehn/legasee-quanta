#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "particle_enhanced.h"
#include "color.h"
#include "unicode_graphics.h"
#include "mouse.h"
#include "forcefield.h"
#include "term.h"

/* Configuration */
#define MAX_PARTICLES 1000
#define WIDTH 120
#define HEIGHT 40
#define FPS 60

/* Global state */
typedef struct {
    ParticleEnhanced particles[MAX_PARTICLES];
    int particle_count;
    ForceFieldManager fields;
    int show_trails;
    int show_fields;
    int paused;
    int quit;
} SimState;

/* Initialize simulation */
void sim_init(SimState *state) {
    state->particle_count = 0;
    state->show_trails = 1;
    state->show_fields = 1;
    state->paused = 0;
    state->quit = 0;
    forcefield_init(&state->fields);
}

/* Spawn particle */
void spawn_particle(SimState *state, float x, float y, float vx, float vy) {
    if (state->particle_count >= MAX_PARTICLES) return;
    
    ParticleEnhanced *p = &state->particles[state->particle_count];
    particle_enhanced_init(p, x, y, vx, vy);
    state->particle_count++;
}

/* Spawn burst of particles */
void spawn_burst(SimState *state, float x, float y, int count) {
    for (int i = 0; i < count && state->particle_count < MAX_PARTICLES; i++) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * M_PI;
        float speed = 5.0f + ((float)rand() / RAND_MAX) * 10.0f;
        float vx = cosf(angle) * speed;
        float vy = sinf(angle) * speed;
        spawn_particle(state, x, y, vx, vy);
    }
}

/* Update physics */
void update_physics(SimState *state, float dt) {
    if (state->paused) return;
    
    float gravity = 20.0f;
    
    for (int i = 0; i < state->particle_count; i++) {
        ParticleEnhanced *p = &state->particles[i];
        
        /* Apply gravity */
        p->vy += gravity * dt;
        
        /* Apply force fields */
        forcefield_apply_all(&state->fields, &p->x, &p->y, &p->vx, &p->vy, dt);
        
        /* Update position */
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        /* Wall collisions */
        if (p->x < 0) { p->x = 0; p->vx = -p->vx * 0.8f; }
        if (p->x >= WIDTH) { p->x = WIDTH - 1; p->vx = -p->vx * 0.8f; }
        if (p->y < 0) { p->y = 0; p->vy = -p->vy * 0.8f; }
        if (p->y >= HEIGHT) { p->y = HEIGHT - 1; p->vy = -p->vy * 0.8f; }
        
        /* Update trail */
        particle_enhanced_update_trail(p);
        
        /* Remove slow particles at bottom */
        if (p->y >= HEIGHT - 1 && fabsf(p->vy) < 1.0f) {
            /* Remove particle by swapping with last */
            state->particles[i] = state->particles[state->particle_count - 1];
            state->particle_count--;
            i--;
        }
    }
}

/* Render frame */
void render_frame(SimState *state) {
    /* Clear screen */
    printf("\033[2J\033[H");
    
    /* Create framebuffer */
    char buffer[HEIGHT][WIDTH * 4];  /* *4 for UTF-8 */
    RGB colors[HEIGHT][WIDTH];
    
    /* Clear buffer */
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            strcpy(&buffer[y][x * 4], " ");
            colors[y][x] = (RGB){0, 0, 0};
        }
    }
    
    /* Render force fields */
    if (state->show_fields) {
        for (int i = 0; i < state->fields.count; i++) {
            ForceField *field = &state->fields.fields[i];
            if (!field->active) continue;
            
            int fx = (int)field->x;
            int fy = (int)field->y;
            
            if (fx >= 0 && fx < WIDTH && fy >= 0 && fy < HEIGHT) {
                const char *glyph = "◎";
                if (field->type == FIELD_ATTRACTOR) glyph = "⊕";
                if (field->type == FIELD_REPELLER) glyph = "⊖";
                if (field->type == FIELD_VORTEX) glyph = "⊗";
                
                strcpy(&buffer[fy][fx * 4], glyph);
                colors[fy][fx] = (RGB){255, 255, 0};  /* Yellow */
            }
        }
    }
    
    /* Render particle trails */
    if (state->show_trails) {
        for (int i = 0; i < state->particle_count; i++) {
            ParticleEnhanced *p = &state->particles[i];
            
            for (int t = 0; t < p->trail_count; t++) {
                int age = (p->trail_index - t - 1 + TRAIL_LENGTH) % TRAIL_LENGTH;
                int tx = (int)p->trail_x[age];
                int ty = (int)p->trail_y[age];
                
                if (tx >= 0 && tx < WIDTH && ty >= 0 && ty < HEIGHT) {
                    const char *glyph = get_trail_glyph(t, TRAIL_LENGTH);
                    strcpy(&buffer[ty][tx * 4], glyph);
                    
                    float speed = sqrtf(p->vx * p->vx + p->vy * p->vy);
                    RGB color = color_from_speed(speed);
                    colors[ty][tx] = color_fade(color, 1.0f - (float)t / TRAIL_LENGTH);
                }
            }
        }
    }
    
    /* Render particles */
    for (int i = 0; i < state->particle_count; i++) {
        ParticleEnhanced *p = &state->particles[i];
        int x = (int)p->x;
        int y = (int)p->y;
        
        if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
            float speed = sqrtf(p->vx * p->vx + p->vy * p->vy);
            const char *glyph = get_particle_glyph(speed, p->vx, p->vy);
            strcpy(&buffer[y][x * 4], glyph);
            colors[y][x] = color_from_energy(p->energy);
        }
    }
    
    /* Output buffer with colors */
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            color_apply(colors[y][x]);
            printf("%s", &buffer[y][x * 4]);
        }
        color_reset();
        printf("\n");
    }
    
    /* Status bar */
    printf("\n");
    printf("Particles: %d | Fields: %d | ", state->particle_count, state->fields.count);
    printf("Controls: [Mouse] Click=Spawn Burst | [A]ttractor [R]epeller [V]ortex | ");
    printf("[T]rails [F]ields [P]ause [C]lear [Q]uit\n");
    
    if (state->paused) {
        printf("*** PAUSED ***\n");
    }
    
    fflush(stdout);
}

/* Process input */
void process_input(SimState *state) {
    /* Check for keyboard input */
    if (term_kbhit()) {
        int ch = term_getch();
        
        switch (ch) {
            case 'q': case 'Q':
                state->quit = 1;
                break;
            case 'p': case 'P':
                state->paused = !state->paused;
                break;
            case 'c': case 'C':
                state->particle_count = 0;
                break;
            case 't': case 'T':
                state->show_trails = !state->show_trails;
                break;
            case 'f': case 'F':
                state->show_fields = !state->show_fields;
                break;
            case 'a': case 'A':
                /* Add attractor at center */
                forcefield_add(&state->fields, WIDTH/2, HEIGHT/2, 100.0f, 50.0f, FIELD_ATTRACTOR);
                break;
            case 'r': case 'R':
                /* Add repeller at center */
                forcefield_add(&state->fields, WIDTH/2, HEIGHT/2, 100.0f, 50.0f, FIELD_REPELLER);
                break;
            case 'v': case 'V':
                /* Add vortex at center */
                forcefield_add(&state->fields, WIDTH/2, HEIGHT/2, 50.0f, 50.0f, FIELD_VORTEX);
                break;
            case 'x': case 'X':
                /* Clear force fields */
                forcefield_clear(&state->fields);
                break;
        }
    }
    
    /* Check for mouse input */
    MouseEvent mouse = mouse_read_event();
    if (mouse.valid && mouse.pressed && mouse.button == 0) {
        /* Left click - spawn burst */
        spawn_burst(state, mouse.x, mouse.y, 20);
    }
}

/* Main loop */
int main(void) {
    srand(time(NULL));
    
    printf("Enhanced ASCII Particle Simulator\n");
    printf("Initializing...\n");
    sleep(1);
    
    /* Initialize terminal */
    if (term_init_raw() != 0) {
        fprintf(stderr, "Failed to initialize terminal\n");
        return 1;
    }
    
    /* Enable mouse */
    mouse_enable();
    
    /* Initialize simulation */
    SimState state;
    sim_init(&state);
    
    /* Spawn initial particles */
    spawn_burst(&state, WIDTH/2, HEIGHT/3, 50);
    
    /* Add initial force field */
    forcefield_add(&state.fields, WIDTH/2, HEIGHT*2/3, 80.0f, 40.0f, FIELD_ATTRACTOR);
    
    /* Main loop */
    double last_time = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    last_time = tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
    
    float dt = 1.0f / FPS;
    
    while (!state.quit) {
        /* Process input */
        process_input(&state);
        
        /* Update physics */
        update_physics(&state, dt);
        
        /* Render */
        render_frame(&state);
        
        /* Frame timing */
        usleep(1000000 / FPS);
    }
    
    /* Cleanup */
    mouse_disable();
    term_restore();
    
    printf("\nSimulation ended. Final particle count: %d\n", state.particle_count);
    
    return 0;
}

