#include "term.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

/* Global variables for terminal state */
static struct termios original_termios;
static int terminal_initialized = 0;
static int quit_flag = 0;

/* Signal handler for graceful cleanup */
static void signal_handler(int sig) {
    (void)sig; /* Suppress unused parameter warning */
    quit_flag = 1;
    term_restore();
    exit(EXIT_SUCCESS);
}

/* Setup signal handlers for cleanup */
void term_setup_signals(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
}

/* Initialize raw terminal mode */
int term_init_raw(void) {
    struct termios new_termios;
    
    /* Get current terminal settings */
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        perror("tcgetattr");
        return -1;
    }
    
    /* Copy original settings and modify for raw mode */
    new_termios = original_termios;
    
    /* Disable canonical mode (line buffering) and echo */
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG);
    
    /* Disable input processing */
    new_termios.c_iflag &= ~(IXON | IXOFF | ICRNL | BRKINT | INLCR | PARMRK | INPCK | ISTRIP);
    
    /* Disable output processing */
    new_termios.c_oflag &= ~(OPOST);
    
    /* Set character size to 8 bits */
    new_termios.c_cflag &= ~(CSIZE | PARENB);
    new_termios.c_cflag |= CS8;
    
    /* Set minimum input and timeout */
    new_termios.c_cc[VMIN] = 0;   /* Return immediately if no data */
    new_termios.c_cc[VTIME] = 0;  /* No timeout */
    
    /* Apply new settings */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        perror("tcsetattr");
        return -1;
    }
    
    terminal_initialized = 1;
    
    /* Setup signal handlers */
    term_setup_signals();
    
    /* Register cleanup function */
    atexit(term_restore);
    
    /* Hide cursor initially */
    term_hide_cursor();
    
    return 0;
}

/* Restore terminal to original state */
void term_restore(void) {
    if (terminal_initialized) {
        /* Show cursor before restoring */
        term_show_cursor();
        
        /* Restore original terminal settings */
        tcsetattr(STDIN_FILENO, TCSANOW, &original_termios);
        
        terminal_initialized = 0;
    }
}

/* Check if key is available (non-blocking) */
int term_kbhit(void) {
    fd_set readfds;
    struct timeval tv;
    
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv) > 0;
}

/* Get character (non-blocking) */
int term_getch(void) {
    char ch;
    ssize_t result;
    
    if (!term_kbhit()) {
        return -1; /* No key available */
    }
    
    result = read(STDIN_FILENO, &ch, 1);
    if (result == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1; /* No data available */
        }
        perror("read");
        return -1;
    }
    
    if (result == 0) {
        return -1; /* EOF */
    }
    
    return (int)(unsigned char)ch;
}

/* ANSI escape sequence helpers */
void term_hide_cursor(void) {
    printf("\033[?25l");
    fflush(stdout);
}

void term_show_cursor(void) {
    printf("\033[?25h");
    fflush(stdout);
}

void term_clear_screen(void) {
    printf("\033[2J");
    fflush(stdout);
}

void term_home(void) {
    printf("\033[H");
    fflush(stdout);
}

void term_set_color_rgb(uint8_t r, uint8_t g, uint8_t b) {
    printf("\033[38;2;%d;%d;%dm", r, g, b);
    fflush(stdout);
}

void term_reset_color(void) {
    printf("\033[0m");
    fflush(stdout);
}

/* Get terminal size with multiple fallback methods */
int term_get_size(int *width, int *height) {
    struct winsize ws;
    
    /* Method 1: Try ioctl TIOCGWINSZ (most reliable) */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        if (width) *width = ws.ws_col;
        if (height) *height = ws.ws_row;
        return 0;
    }
    
    /* Method 2: Try ioctl on STDIN */
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        if (width) *width = ws.ws_col;
        if (height) *height = ws.ws_row;
        return 0;
    }
    
    /* Method 3: Try environment variables */
    const char *cols = getenv("COLUMNS");
    const char *lines = getenv("LINES");
    
    if (cols && lines) {
        int w = atoi(cols);
        int h = atoi(lines);
        if (w > 0 && h > 0) {
            if (width) *width = w;
            if (height) *height = h;
            return 0;
        }
    }
    
    /* Method 4: Try stty size command (fallback) */
    FILE *stty = popen("stty size 2>/dev/null", "r");
    if (stty) {
        char line[32];
        if (fgets(line, sizeof(line), stty)) {
            int h, w;
            if (sscanf(line, "%d %d", &h, &w) == 2 && w > 0 && h > 0) {
                if (width) *width = w;
                if (height) *height = h;
                pclose(stty);
                return 0;
            }
        }
        pclose(stty);
    }
    
    /* Method 5: Reasonable defaults */
    if (width) *width = 80;
    if (height) *height = 24;
    return -1;
}

/* Get quit flag status */
int term_should_quit(void) {
    return quit_flag;
}

/* ===== ERROR-AWARE TERMINAL FUNCTIONS ===== */

/* Initialize raw terminal mode with error handling */
Error term_init_raw_with_error(void) {
    struct termios new_termios;
    
    /* Get current terminal settings */
    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to get terminal attributes");
    }
    
    /* Copy original settings and modify for raw mode */
    new_termios = original_termios;
    
    /* Disable canonical mode (line buffering) and echo */
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ISIG);
    
    /* Disable input processing */
    new_termios.c_iflag &= ~(IXON | IXOFF | ICRNL | BRKINT | INLCR | PARMRK | INPCK | ISTRIP);
    
    /* Disable output processing */
    new_termios.c_oflag &= ~(OPOST);
    
    /* Set character size to 8 bits */
    new_termios.c_cflag &= ~(CSIZE | PARENB);
    new_termios.c_cflag |= CS8;
    
    /* Set minimum input and timeout */
    new_termios.c_cc[VMIN] = 0;   /* Return immediately if no data */
    new_termios.c_cc[VTIME] = 0;  /* No timeout */
    
    /* Apply new settings */
    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) == -1) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to set terminal attributes");
    }
    
    terminal_initialized = 1;
    
    /* Setup signal handlers */
    term_setup_signals();
    
    /* Register cleanup function */
    atexit(term_restore);
    
    /* Hide cursor initially */
    term_hide_cursor();
    
    return (Error){SUCCESS};
}

/* Get terminal size with error handling */
Error term_get_size_with_error(int *width_out, int *height_out) {
    ERROR_CHECK(width_out != NULL, ERROR_NULL_POINTER, "Width output pointer cannot be NULL");
    ERROR_CHECK(height_out != NULL, ERROR_NULL_POINTER, "Height output pointer cannot be NULL");
    
    struct winsize ws;
    
    /* Method 1: Try ioctl TIOCGWINSZ (most reliable) */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *width_out = ws.ws_col;
        *height_out = ws.ws_row;
        return (Error){SUCCESS};
    }
    
    /* Method 2: Try ioctl on STDIN */
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        *width_out = ws.ws_col;
        *height_out = ws.ws_row;
        return (Error){SUCCESS};
    }
    
    /* Method 3: Try environment variables */
    const char *cols = getenv("COLUMNS");
    const char *lines = getenv("LINES");
    
    if (cols && lines) {
        int w = atoi(cols);
        int h = atoi(lines);
        if (w > 0 && h > 0) {
            *width_out = w;
            *height_out = h;
            return (Error){SUCCESS};
        }
    }
    
    /* Method 4: Try stty size command (fallback) */
    FILE *stty = popen("stty size 2>/dev/null", "r");
    if (stty) {
        char line[32];
        if (fgets(line, sizeof(line), stty)) {
            int h, w;
            if (sscanf(line, "%d %d", &h, &w) == 2 && w > 0 && h > 0) {
                *width_out = w;
                *height_out = h;
                pclose(stty);
                return (Error){SUCCESS};
            }
        }
        pclose(stty);
    }
    
    /* Method 5: Reasonable defaults */
    *width_out = 80;
    *height_out = 24;
    return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to determine terminal size, using defaults");
}

/* Get character with error handling */
Error term_getch_with_error(int *ch_out) {
    ERROR_CHECK(ch_out != NULL, ERROR_NULL_POINTER, "Character output pointer cannot be NULL");
    
    if (!term_kbhit()) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "No key available");
    }
    
    char ch;
    ssize_t result = read(STDIN_FILENO, &ch, 1);
    
    if (result == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ERROR_CREATE(ERROR_SYSTEM_ERROR, "No data available");
        }
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "Failed to read from terminal");
    }
    
    if (result == 0) {
        return ERROR_CREATE(ERROR_SYSTEM_ERROR, "End of file reached");
    }
    
    *ch_out = (int)(unsigned char)ch;
    return (Error){SUCCESS};
}

/* Check if terminal is initialized with error handling */
Error term_is_initialized_with_error(int *initialized_out) {
    ERROR_CHECK(initialized_out != NULL, ERROR_NULL_POINTER, "Initialized output pointer cannot be NULL");
    
    *initialized_out = terminal_initialized;
    return (Error){SUCCESS};
} 
