#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <getopt.h>
#include "term.h"
#include "render.h"
#include "sim.h"
#include "input.h"

/* Configuration structure */
typedef struct {
    int max_particles;
    int target_fps;
    int width;
    int height;
    int auto_size;
} Config;

/* Default configuration */
static const Config DEFAULT_CONFIG = {
    .max_particles = 2000,
    .target_fps = 60,
    .width = 0,
    .height = 0,
    .auto_size = 1
};

/* FPS calculation helpers */
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

static double calculate_fps(double *last_time, int *frame_count) {
    double current_time = get_time_ms();
    double delta = current_time - *last_time;
    
    if (delta >= 1000.0) { /* Update FPS every second */
        double fps = *frame_count * 1000.0 / delta;
        *last_time = current_time;
        *frame_count = 0;
        return fps;
    }
    
    (*frame_count)++;
    return -1.0; /* No update needed */
}

/* CLI argument parsing */
static void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("Interactive ASCII Particle Physics Simulator\n\n");
    printf("Options:\n");
    printf("  -p, --max-particles <count>  Maximum particle count (default: %d)\n", DEFAULT_CONFIG.max_particles);
    printf("  -f, --fps <rate>            Target frame rate (default: %d)\n", DEFAULT_CONFIG.target_fps);
    printf("  -s, --size <width>x<height> Terminal size (default: auto-detect)\n");
    printf("  -h, --help                  Show this help message\n");
    printf("  -v, --version               Show version information\n\n");
    printf("Controls:\n");
    printf("  WASD: Wind control    Space: Spawn burst    C: Clear particles\n");
    printf("  G: Toggle gravity     P: Pause/Resume       +/-: Adjust gravity\n");
    printf("  Q: Quit              H: Toggle HUD         R: Reset simulation\n");
}

static void print_version(void) {
    printf("ASCII Particle Simulator v1.0.0\n");
    printf("Built with C11, optimized for real-time terminal graphics\n");
}

static int parse_size_string(const char *size_str, int *width, int *height) {
    if (!size_str || !width || !height) return -1;
    
    char *x_pos = strchr(size_str, 'x');
    if (!x_pos) return -1;
    
    *x_pos = '\0';
    int w = atoi(size_str);
    int h = atoi(x_pos + 1);
    *x_pos = 'x'; /* Restore original string */
    
    if (w <= 0 || h <= 0 || w > 200 || h > 100) return -1;
    
    *width = w;
    *height = h;
    return 0;
}

static Config parse_arguments(int argc, char *argv[]) {
    Config config = DEFAULT_CONFIG;
    
    static struct option long_options[] = {
        {"max-particles", required_argument, 0, 'p'},
        {"fps", required_argument, 0, 'f'},
        {"size", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "p:f:s:hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'p':
                config.max_particles = atoi(optarg);
                if (config.max_particles <= 0 || config.max_particles > 10000) {
                    fprintf(stderr, "Error: Invalid particle count %s (1-10000)\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
                
            case 'f':
                config.target_fps = atoi(optarg);
                if (config.target_fps <= 0 || config.target_fps > 120) {
                    fprintf(stderr, "Error: Invalid FPS %s (1-120)\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
                
            case 's':
                if (parse_size_string(optarg, &config.width, &config.height) != 0) {
                    fprintf(stderr, "Error: Invalid size format %s (use WxH, e.g., 80x24)\n", optarg);
                    exit(EXIT_FAILURE);
                }
                config.auto_size = 0;
                break;
                
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
                
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                
            case '?':
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
                
            default:
                abort();
        }
    }
    
    return config;
}

/* Terminal size detection with fallbacks */
static int detect_terminal_size(int *width, int *height) {
    /* Try ioctl method first */
    if (term_get_size(width, height) == 0) {
        return 0;
    }
    
    /* Fallback to environment variables */
    const char *cols = getenv("COLUMNS");
    const char *lines = getenv("LINES");
    
    if (cols && lines) {
        *width = atoi(cols);
        *height = atoi(lines);
        if (*width > 0 && *height > 0) {
            return 0;
        }
    }
    
    /* Final fallback to reasonable defaults */
    *width = 80;
    *height = 24;
    return -1;
}

int main(int argc, char *argv[]) {
    Config config = parse_arguments(argc, argv);
    int width, height;
    
    printf("ASCII Particle Physics Simulator v1.0.0\n");
    printf("Real-time terminal graphics with interactive physics\n\n");
    
    /* Detect or use specified terminal size */
    if (config.auto_size) {
        if (detect_terminal_size(&width, &height) == 0) {
            printf("Detected terminal size: %dx%d\n", width, height);
        } else {
            printf("Using default size: %dx%d\n", width, height);
        }
    } else {
        width = config.width;
        height = config.height;
        printf("Using specified size: %dx%d\n", width, height);
    }
    
    /* Validate terminal size */
    if (width < 20 || height < 10) {
        fprintf(stderr, "Error: Terminal too small (%dx%d). Minimum 20x10 required.\n", width, height);
        return EXIT_FAILURE;
    }
    
    printf("Configuration: %d particles, %d FPS target\n", config.max_particles, config.target_fps);
    
    /* Initialize terminal in raw mode */
    if (term_init_raw() != 0) {
        fprintf(stderr, "Failed to initialize terminal\n");
        return EXIT_FAILURE;
    }
    
    /* Create renderer */
    Renderer *renderer = renderer_create(width, height);
    if (!renderer) {
        fprintf(stderr, "Failed to create renderer\n");
        term_restore();
        return EXIT_FAILURE;
    }
    
    /* Create simulation with configured particle limit */
    Simulation *sim = sim_create(config.max_particles, width, height);
    if (!sim) {
        fprintf(stderr, "Failed to create simulation\n");
        renderer_destroy(renderer);
        term_restore();
        return EXIT_FAILURE;
    }
    
    /* Initialize UI state */
    UIState ui;
    input_init_state(&ui);
    
    printf("Interactive simulation ready!\n");
    printf("%s\n", input_get_help_text());
    printf("Press any key to start, 'q' to quit...\n");
    
    /* Wait for key press */
    while (!term_kbhit()) {
        usleep(10000);
    }
    int ch = term_getch();
    if (ch == 'q' || ch == 'Q') {
        sim_destroy(sim);
        renderer_destroy(renderer);
        term_restore();
        return EXIT_SUCCESS;
    }
    
    /* Clear screen and start interactive demo */
    term_clear_screen();
    
    /* Spawn initial burst */
    sim_spawn_burst(sim, width / 2, height / 3, 100, BURST_SPREAD);
    
    /* Main interactive loop */
    int frames = 0;
    const float dt = 1.0f / (float)config.target_fps; /* Fixed timestep based on target FPS */
    
    /* FPS calculation variables */
    double last_fps_time = get_time_ms();
    int fps_frame_count = 0;
    double current_fps = 0.0;
    
    /* Performance monitoring */
    double frame_times[60] = {0}; /* Store last 60 frame times */
    int frame_time_index = 0;
    double target_frame_time = 1000.0 / config.target_fps;
    
    /* Memory usage tracking */
    #ifdef __linux__
    size_t initial_memory = 0;
    FILE *meminfo = fopen("/proc/self/status", "r");
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                sscanf(line, "VmRSS: %zu", &initial_memory);
                break;
            }
        }
        fclose(meminfo);
    }
    #else
    (void)0; /* Suppress unused variable warning on non-Linux */
    #endif
    
    printf("Starting simulation loop...\n");
    
    while (!input_should_quit(&ui)) {
        double frame_start = get_time_ms();
        
        /* Process input */
        input_process_frame(sim, &ui);
        
        /* Step physics simulation (if not paused) */
        if (!input_is_paused(&ui)) {
            sim_step(sim, dt);
        }
        
        /* Clear renderer */
        renderer_clear(renderer);
        
        /* Render all particles with optimized loop */
        int particle_count = sim_get_particle_count(sim);
        for (int i = 0; i < particle_count; i++) {
            const Particle *p = sim_get_particle(sim, i);
            if (p) {
                int x = (int)roundf(p->x);
                int y = (int)roundf(p->y);
                
                /* Bounds checking for performance */
                if (x < 0 || x >= width || y < 0 || y >= height) continue;
                
                /* Choose glyph based on speed */
                float speed = sim_get_particle_speed(p);
                char glyph;
                if (speed < 5.0f) glyph = '.';
                else if (speed < 15.0f) glyph = '*';
                else glyph = '+';
                
                /* Get color based on speed */
                uint32_t color = sim_speed_to_color(speed);
                
                /* Plot particle */
                renderer_plot(renderer, x, y, glyph, color);
            }
        }
        
        /* Render HUD if enabled */
        if (input_show_hud(&ui)) {
            /* Calculate FPS */
            double fps_result = calculate_fps(&last_fps_time, &fps_frame_count);
            if (fps_result >= 0.0) {
                current_fps = fps_result;
            }
            
            /* Status line with enhanced info */
            char status_text[256];
            input_get_status_text(sim, &ui, status_text, sizeof(status_text));
            renderer_draw_text(renderer, 0, 0, status_text, rgb_to_color(255, 255, 255));
            
            /* Performance info */
            char perf_text[128];
            snprintf(perf_text, sizeof(perf_text), 
                    "FPS: %.1f/%d | Particles: %d/%d | Frame: %d", 
                    current_fps, config.target_fps, particle_count, config.max_particles, frames);
            renderer_draw_text(renderer, 0, 1, perf_text, rgb_to_color(200, 200, 200));
            
            /* Physics parameters */
            char physics_text[128];
            float gravity = sim_get_gravity(sim);
            float windx, windy;
            sim_get_wind(sim, &windx, &windy);
            snprintf(physics_text, sizeof(physics_text), 
                    "Gravity: %.1f | Wind: (%.1f, %.1f) | Size: %dx%d", 
                    gravity, windx, windy, width, height);
            renderer_draw_text(renderer, 0, 2, physics_text, rgb_to_color(150, 255, 150));
            
            /* Memory usage (if available) */
            #ifdef __linux__
            if (initial_memory > 0) {
                size_t current_memory = 0;
                FILE *meminfo = fopen("/proc/self/status", "r");
                if (meminfo) {
                    char line[256];
                    while (fgets(line, sizeof(line), meminfo)) {
                        if (strncmp(line, "VmRSS:", 6) == 0) {
                            sscanf(line, "VmRSS: %zu", &current_memory);
                            break;
                        }
                    }
                    fclose(meminfo);
                }
                if (current_memory > 0) {
                    char mem_text[64];
                    snprintf(mem_text, sizeof(mem_text), "Memory: %zu KB", current_memory);
                    renderer_draw_text(renderer, 0, 3, mem_text, rgb_to_color(255, 200, 150));
                }
            }
            #endif
            
            /* Help text (first line only) */
            const char *help = input_get_help_text();
            char help_line[width + 1];
            strncpy(help_line, help, width);
            help_line[width] = '\0';
            renderer_draw_text(renderer, 0, height - 1, help_line, rgb_to_color(150, 150, 150));
        }
        
        /* Flush to screen */
        renderer_flush(renderer);
        
        frames++;
        
        /* Frame timing and pacing */
        double frame_end = get_time_ms();
        double frame_duration = frame_end - frame_start;
        
        /* Store frame time for averaging */
        frame_times[frame_time_index] = frame_duration;
        frame_time_index = (frame_time_index + 1) % 60;
        
        /* Calculate target sleep time for target FPS */
        double sleep_time = target_frame_time - frame_duration;
        
        if (sleep_time > 0) {
            usleep((useconds_t)(sleep_time * 1000));
        }
    }
    
    printf("\nSimulation ended. Rendered %d frames.\n", frames);
    printf("Average FPS: %.1f (target: %d)\n", current_fps, config.target_fps);
    printf("Final particle count: %d\n", sim_get_particle_count(sim));
    
    /* Cleanup */
    sim_destroy(sim);
    renderer_destroy(renderer);
    term_restore();
    
    return EXIT_SUCCESS;
} 
