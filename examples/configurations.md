# ASCII Particle Simulator - Configuration Examples

This document provides example configurations for different use cases and performance requirements.

## Quick Start Configurations

### Low Resource Mode

For older systems or limited resources:

```bash
./sim --max-particles 500 --fps 30 --size 60x20
```

- **Use case**: Older terminals, embedded systems, SSH connections
- **Performance**: ~30 FPS, ~2MB memory usage
- **Best for**: Basic particle visualization

### Standard Mode

Default configuration for most systems:

```bash
./sim --max-particles 2000 --fps 60 --size 80x24
```

- **Use case**: Modern terminals, development work
- **Performance**: ~60 FPS, ~8MB memory usage
- **Best for**: Interactive physics experimentation

### High Performance Mode

For powerful systems and maximum visual impact:

```bash
./sim --max-particles 5000 --fps 120 --size 120x40
```

- **Use case**: Modern terminals, demonstrations, high-end systems
- **Performance**: ~120 FPS, ~15MB memory usage
- **Best for**: Impressive visual demonstrations

## Specialized Configurations

### Educational Mode

Optimized for teaching physics concepts:

```bash
./sim --max-particles 1000 --fps 60 --size 100x30
```

- **Features**: Clear particle visibility, moderate complexity
- **Best for**: Physics education, concept demonstration

### Performance Testing Mode

For benchmarking and stress testing:

```bash
./sim --max-particles 10000 --fps 60 --size 80x24
```

- **Features**: Maximum particle count, standard resolution
- **Best for**: System performance testing, stress testing

### Mobile Terminal Mode

Optimized for mobile terminals and small screens:

```bash
./sim --max-particles 300 --fps 30 --size 40x12
```

- **Features**: Minimal resource usage, small display
- **Best for**: Mobile terminals, embedded displays

## Terminal-Specific Configurations

### Classic Terminal (80x24)

Traditional terminal size:

```bash
./sim --size 80x24 --max-particles 1500 --fps 60
```

### Wide Terminal (120x36)

Modern wide terminals:

```bash
./sim --size 120x36 --max-particles 3000 --fps 60
```

### Square Terminal (60x60)

Square aspect ratio:

```bash
./sim --size 60x60 --max-particles 2000 --fps 60
```

## Performance Tuning

### For Maximum FPS

```bash
./sim --fps 120 --max-particles 1000 --size 60x20
```

### For Maximum Particles

```bash
./sim --max-particles 8000 --fps 30 --size 100x30
```

### For Balanced Performance

```bash
./sim --max-particles 2000 --fps 60 --size 80x24
```

## Troubleshooting Configurations

### If Terminal is Too Slow

```bash
./sim --max-particles 500 --fps 30 --size 60x20
```

### If Terminal Shows Garbage

```bash
./sim --size 80x24 --max-particles 100 --fps 30
```

### If Memory Usage is High

```bash
./sim --max-particles 1000 --fps 30 --size 60x20
```

## Environment-Specific Recommendations

### SSH Connections

```bash
./sim --max-particles 1000 --fps 30 --size 80x24
```

### Local Terminal

```bash
./sim --max-particles 3000 --fps 60 --size 120x36
```

### Development/Testing

```bash
./sim --max-particles 500 --fps 60 --size 80x24
```

## Advanced Usage

### Batch Testing Multiple Configurations

```bash
# Test different particle counts
for particles in 500 1000 2000 5000; do
    echo "Testing with $particles particles..."
    timeout 10 ./sim --max-particles $particles --fps 60
done
```

### Performance Monitoring

```bash
# Monitor memory usage while running
./sim --max-particles 2000 --fps 60 &
PID=$!
while kill -0 $PID 2>/dev/null; do
    ps -o rss= -p $PID
    sleep 1
done
```

### Automated Testing

Use the provided test script:

```bash
./examples/performance_test.sh
```

## Configuration Tips

1. **Start Small**: Begin with low particle counts and increase gradually
2. **Monitor FPS**: Use the HUD (press H) to monitor actual performance
3. **Test Terminal**: Ensure your terminal supports 24-bit color
4. **Memory Usage**: Monitor memory usage on resource-constrained systems
5. **Network Latency**: Reduce FPS for remote connections

## Performance Benchmarks

| Configuration | Particles | FPS | Memory | Use Case |
|---------------|-----------|-----|--------|----------|
| Low Resource | 500 | 30 | ~2MB | Old systems |
| Standard | 2000 | 60 | ~8MB | General use |
| High Perf | 5000 | 120 | ~15MB | Modern systems |
| Extreme | 10000 | 60 | ~25MB | Stress testing |

*Note: Actual performance may vary based on system specifications and terminal capabilities.*
