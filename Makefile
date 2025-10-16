CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -pedantic -D_GNU_SOURCE
TARGET = sim
SRCDIR = src
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:.c=.o)

# Generate object files from sources and ensure new modules are included
NEW_MODULES = src/error.o
ALL_OBJECTS = $(SOURCES:.c=.o)
EXISTING_MODULES = $(filter-out $(NEW_MODULES), $(ALL_OBJECTS))
# Temporarily exclude problematic modules: src/config.o src/log.o
PROBLEMATIC_MODULES = src/config.o src/log.o
FILTERED_MODULES = $(filter-out $(PROBLEMATIC_MODULES), $(EXISTING_MODULES))
OBJECTS = $(FILTERED_MODULES) $(NEW_MODULES)

# Performance optimization flags
OPT_CFLAGS = -O3 -march=native -ffast-math -funroll-loops

# SIMD-specific flags
SIMD_CFLAGS = -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2
AVX_CFLAGS = -mavx -mavx2 -mfma
NEON_CFLAGS = -mfpu=neon -mfloat-abi=hard
DEBUG_CFLAGS = -g -O0 -DDEBUG
PROFILE_CFLAGS = -pg -O2

.PHONY: all clean run debug profile test install uninstall help demo_enhanced

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Optimized build
optimized: CFLAGS += $(OPT_CFLAGS)
optimized: $(TARGET)

# Debug build
debug: CFLAGS = $(DEBUG_CFLAGS)
debug: $(TARGET)

# Profile build
profile: CFLAGS += $(PROFILE_CFLAGS)
profile: $(TARGET)

clean:
	rm -f $(OBJECTS) $(PROBLEMATIC_MODULES) $(TARGET) gmon.out simd_test

run: $(TARGET)
	./$(TARGET)

# Test with different configurations
test-small: $(TARGET)
	./$(TARGET) --max-particles 500 --size 60x20

test-large: $(TARGET)
	./$(TARGET) --max-particles 3000 --size 120x40

test-performance: $(TARGET)
	./$(TARGET) --max-particles 5000 --fps 120

# Memory leak testing
test-memory: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET)

# Performance profiling
profile-run: profile
	./$(TARGET) --max-particles 2000
	gprof $(TARGET) gmon.out > profile.txt
	@echo "Profile results saved to profile.txt"

# Object pool testing
test-pool: $(TARGET)
	$(CC) $(CFLAGS) -o pool_test examples/pool_test_simple.c src/pool.c -lm -pthread
	./pool_test

# SIMD testing - platform agnostic
simd_test: clean
	$(CC) $(CFLAGS) -o simd_test examples/simd_test.c src/simd.c src/particle.c -lm

# Improvement testing
improvement_test: clean
	$(CC) $(CFLAGS) -o improvement_test examples/improvement_test.c src/error.c src/config.c src/log.c -lm

# Pool error handling integration test
pool_error_test: clean
	$(CC) $(CFLAGS) -o pool_error_test examples/pool_error_test.c src/error.c src/pool.c src/particle.c -lm

# Comprehensive integration test
integration_test: clean
	$(CC) $(CFLAGS) -o integration_test examples/integration_test.c src/error.c src/pool.c src/simd.c src/sim.c src/term.c src/render.c src/input.c src/particle.c -lm

# SIMD testing with x86-specific flags (for x86 platforms only)
test-simd: $(TARGET)
	$(CC) $(CFLAGS) $(SIMD_CFLAGS) -o simd_test examples/simd_test.c src/simd.c src/particle.c -lm
	./simd_test

install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/$(TARGET)"

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled from /usr/local/bin/$(TARGET)"

# Enhanced demo with new features
demo_enhanced: clean
	$(CC) $(CFLAGS) -o demo_enhanced src/demo_enhanced.c src/term.c -lm

help:
	@echo "Available targets:"
	@echo "  all          - Build the simulator (default)"
	@echo "  optimized    - Build with maximum optimization"
	@echo "  debug        - Build with debug symbols"
	@echo "  profile      - Build for profiling"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Build and run"
	@echo "  test-small   - Test with small configuration"
	@echo "  test-large   - Test with large configuration"
	@echo "  test-performance - Test with high performance settings"
	@echo "  test-memory  - Run memory leak test with valgrind"
	@echo "  profile-run  - Run and generate performance profile"
	@echo "  simd_test    - Build and run SIMD capability tests"
	@echo "  test-simd    - Test with x86-specific SIMD flags"
	@echo "  improvement_test - Test new error/config/log systems"
	@echo "  pool_error_test - Test pool error handling integration"
	@echo "  integration_test - Test all error handling systems together"
	@echo "  install      - Install to system"
	@echo "  uninstall    - Remove from system"
	@echo "  help         - Show this help"
	@echo "  demo_enhanced - Build enhanced demo with Unicode, mouse, trails, force fields" 