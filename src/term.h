#ifndef TERM_H
#define TERM_H

#include <stdint.h>

/* Terminal control functions */
int term_init_raw(void);
void term_restore(void);
int term_kbhit(void);
int term_getch(void);
void term_setup_signals(void);
int term_should_quit(void);

/* ANSI escape sequence helpers */
void term_hide_cursor(void);
void term_show_cursor(void);
void term_clear_screen(void);
void term_home(void);
void term_set_color_rgb(uint8_t r, uint8_t g, uint8_t b);
void term_reset_color(void);

/* Terminal size detection */
int term_get_size(int *width, int *height);

#endif /* TERM_H */ 
