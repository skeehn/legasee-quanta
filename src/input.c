#include "input.h"
#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

/* Initialize UI state with default values */
void input_init_state(UIState *ui) {
    if (!ui) return;
    
    ui->paused = false;
    ui->quit = false;
    ui->show_hud = true;
    ui->input_state = 0;
}

/* Handle a single key press and update simulation/UI state */
void input_handle_key(int key, Simulation *sim, UIState *ui) {
    if (!sim || !ui) return;
    
    switch (key) {
        case KEY_W:
            /* Wind up (negative Y) */
            sim->windy -= WIND_STEP;
            break;
            
        case KEY_S:
            /* Wind down (positive Y) */
            sim->windy += WIND_STEP;
            break;
            
        case KEY_A:
            /* Wind left (negative X) */
            sim->windx -= WIND_STEP;
            break;
            
        case KEY_D:
            /* Wind right (positive X) */
            sim->windx += WIND_STEP;
            break;
            
        case KEY_G:
            /* Toggle gravity */
            if (sim->gravity != 0.0f) {
                sim->gravity = 0.0f;
            } else {
                sim->gravity = GRAVITY_DEFAULT;
            }
            break;
            
        case KEY_SPACE:
            /* Spawn particle burst at center */
            sim_spawn_burst(sim, sim->width / 2, sim->height / 3, 
                          BURST_COUNT, BURST_SPREAD);
            break;
            
        case KEY_C:
            /* Clear all particles */
            sim_clear(sim);
            break;
            
        case KEY_P:
            /* Toggle pause */
            ui->paused = !ui->paused;
            break;
            
        case KEY_PLUS:
        case '=': /* Also handle equals key for convenience */
            /* Increase gravity strength */
            sim->gravity *= (1.0f + GRAVITY_STEP);
            break;
            
        case KEY_MINUS:
        case '_': /* Also handle underscore key for convenience */
            /* Decrease gravity strength */
            sim->gravity *= (1.0f - GRAVITY_STEP);
            break;
            
        case KEY_Q:
        case 'Q':
            /* Quit application */
            ui->quit = true;
            break;
            
        case 'h':
        case 'H':
            /* Toggle HUD visibility */
            ui->show_hud = !ui->show_hud;
            break;
            
        case 'r':
        case 'R':
            /* Reset simulation */
            sim_clear(sim);
            sim->windx = 0.0f;
            sim->windy = 0.0f;
            sim->gravity = GRAVITY_DEFAULT;
            break;
            
        case '1':
            /* Spawn small burst */
            sim_spawn_burst(sim, sim->width / 2, sim->height / 3, 
                          50, BURST_SPREAD);
            break;
            
        case '2':
            /* Spawn medium burst */
            sim_spawn_burst(sim, sim->width / 2, sim->height / 3, 
                          100, BURST_SPREAD);
            break;
            
        case '3':
            /* Spawn large burst */
            sim_spawn_burst(sim, sim->width / 2, sim->height / 3, 
                          200, BURST_SPREAD);
            break;
            
        default:
            /* Unknown key - ignore */
            break;
    }
    
    /* Validate and clamp physics parameters */
    if (sim->gravity < 0.0f) sim->gravity = 0.0f;
    if (sim->gravity > 100.0f) sim->gravity = 100.0f;
    
    /* Clamp wind to reasonable range */
    if (sim->windx < -50.0f) sim->windx = -50.0f;
    if (sim->windx > 50.0f) sim->windx = 50.0f;
    if (sim->windy < -50.0f) sim->windy = -50.0f;
    if (sim->windy > 50.0f) sim->windy = 50.0f;
}

/* Process input for the current frame */
void input_process_frame(Simulation *sim, UIState *ui) {
    if (!sim || !ui) return;
    
    /* Check for quit signal from terminal */
    if (term_should_quit()) {
        ui->quit = true;
        return;
    }
    
    /* Process all available keys (non-blocking) */
    while (term_kbhit()) {
        int key = term_getch();
        if (key != -1) {
            input_handle_key(key, sim, ui);
        }
    }
}

/* Get control help text */
const char *input_get_help_text(void) {
    return "Controls: WASD=Wind, G=Gravity Toggle, Space=Burst, C=Clear, "
           "P=Pause, +/-=Gravity, Q=Quit, H=HUD, R=Reset, 1/2/3=Burst Size";
}

/* Get status text for current simulation state */
void input_get_status_text(const Simulation *sim, const UIState *ui, char *buffer, size_t buffer_size) {
    if (!sim || !ui || !buffer) return;
    
    const char *pause_text = ui->paused ? "PAUSED" : "RUNNING";
    const char *gravity_text = sim->gravity > 0.0f ? "ON" : "OFF";
    
    snprintf(buffer, buffer_size,
             "Status: %s | Gravity: %s (%.1f) | Wind: (%.1f, %.1f) | Particles: %d",
             pause_text, gravity_text, sim->gravity, sim->windx, sim->windy,
             sim_get_particle_count(sim));
}

/* Check if simulation should be paused */
bool input_is_paused(const UIState *ui) {
    return ui ? ui->paused : false;
}

/* Check if application should quit */
bool input_should_quit(const UIState *ui) {
    return ui ? ui->quit : false;
}

/* Check if HUD should be shown */
bool input_show_hud(const UIState *ui) {
    return ui ? ui->show_hud : true;
} 
