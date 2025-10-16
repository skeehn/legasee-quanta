#ifndef MOUSE_H
#define MOUSE_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

/* Mouse event structure */
typedef struct {
    int x, y;           /* Mouse coordinates */
    int button;         /* 0=left, 1=middle, 2=right, 3=release */
    int pressed;        /* 1=pressed, 0=released */
    int valid;          /* 1=valid event, 0=invalid */
} MouseEvent;

/* Enable mouse tracking in terminal */
static inline void mouse_enable(void) {
    /* Enable mouse tracking modes */
    printf("\033[?1000h");  /* Basic mouse tracking */
    printf("\033[?1002h");  /* Button event tracking */
    printf("\033[?1015h");  /* Extended coordinates */
    printf("\033[?1006h");  /* SGR mouse mode */
    fflush(stdout);
}

/* Disable mouse tracking */
static inline void mouse_disable(void) {
    printf("\033[?1006l");
    printf("\033[?1015l");
    printf("\033[?1002l");
    printf("\033[?1000l");
    fflush(stdout);
}

/* Parse SGR mouse event: \033[<B;X;Y(M/m) */
static inline MouseEvent mouse_parse_event(const char *seq) {
    MouseEvent event = {0};
    event.valid = 0;
    
    if (!seq || seq[0] != '\033' || seq[1] != '[' || seq[2] != '<') {
        return event;
    }
    
    int button, x, y;
    char action;
    
    if (sscanf(seq + 3, "%d;%d;%d%c", &button, &x, &y, &action) == 4) {
        event.button = button & 3;  /* Extract button number */
        event.x = x - 1;  /* Convert to 0-based */
        event.y = y - 1;
        event.pressed = (action == 'M') ? 1 : 0;
        event.valid = 1;
    }
    
    return event;
}

/* Read mouse event from stdin (non-blocking) */
static inline MouseEvent mouse_read_event(void) {
    MouseEvent event = {0};
    event.valid = 0;
    
    char buf[32] = {0};
    int i = 0;
    int c;
    
    /* Check if data available */
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_cc[VMIN] = 0;
    new_tio.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);
    
    /* Read escape sequence */
    while (i < 31 && (c = getchar()) != EOF) {
        buf[i++] = c;
        if (c == 'M' || c == 'm') break;
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    
    if (i > 0) {
        event = mouse_parse_event(buf);
    }
    
    return event;
}

#endif /* MOUSE_H */

