#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include "sim.h"

/* UI state structure */
typedef struct {
    bool paused;
    bool quit;
    bool show_hud;
    int input_state;
} UIState;

/* Input handling functions */
void input_handle_key(int key, Simulation *sim, UIState *ui);
void input_process_frame(Simulation *sim, UIState *ui);
void input_init_state(UIState *ui);

/* Control mapping */
#define KEY_W 119
#define KEY_A 97
#define KEY_S 115
#define KEY_D 100
#define KEY_G 103
#define KEY_SPACE 32
#define KEY_C 99
#define KEY_P 112
#define KEY_PLUS 43
#define KEY_MINUS 45
#define KEY_Q 113

/* Control constants */
#define WIND_STEP 3.0f
#define GRAVITY_DEFAULT 30.0f
#define GRAVITY_STEP 0.1f
#define BURST_COUNT 200
#define BURST_SPREAD 1.047f  /* π/3 radians */

/* Utility functions */
const char *input_get_help_text(void);
void input_get_status_text(const Simulation *sim, const UIState *ui, char *buffer, size_t buffer_size);
bool input_is_paused(const UIState *ui);
bool input_should_quit(const UIState *ui);
bool input_show_hud(const UIState *ui);

#endif /* INPUT_H */ 
