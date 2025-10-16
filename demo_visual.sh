#!/bin/bash

# Visual demonstration of enhanced features

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║     Enhanced ASCII Particle Simulator - Feature Showcase      ║"
echo "╚════════════════════════════════════════════════════════════════╝"
echo ""

# Test UTF-8 support
echo "1. Unicode Graphics Support:"
echo "   Dots: · • ● ⬤"
echo "   Arrows: ← ↑ → ↓ ↖ ↗ ↘ ↙"
echo "   Blocks: ▁ ▂ ▃ ▄ ▅ ▆ ▇ █"
echo "   Force Fields: ⊕ ⊖ ⊗"
echo ""

# Test true color
echo "2. True Color (24-bit RGB) Support:"
printf "   Speed gradient: "
for i in {0..20}; do
    r=$((i * 12))
    g=$((255 - i * 6))
    b=$((128 - i * 6))
    printf "\033[38;2;${r};${g};${b}m●\033[0m"
done
echo ""
echo ""

# Test features
echo "3. New Features:"
echo "   ✓ Mouse support for interactive particle spawning"
echo "   ✓ Particle trails with motion blur effects"
echo "   ✓ Interactive force fields (attractors, repellers, vortex)"
echo "   ✓ Enhanced visual feedback with colors"
echo "   ✓ Real-time physics with force field interactions"
echo ""

echo "4. Build & Run:"
echo "   $ make demo_enhanced"
echo "   $ ./demo_enhanced"
echo ""

echo "5. Quick Controls:"
echo "   Mouse Click → Spawn particles"
echo "   A → Attractor  |  R → Repeller  |  V → Vortex"
echo "   T → Trails     |  F → Fields    |  P → Pause"
echo "   C → Clear      |  Q → Quit"
echo ""

echo "╔════════════════════════════════════════════════════════════════╗"
echo "║  Ready to run! Execute: ./demo_enhanced                        ║"
echo "╚════════════════════════════════════════════════════════════════╝"
