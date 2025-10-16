# Quick Start: Enhanced Particle Simulator 🚀

## Build & Run (30 seconds)

```bash
# Navigate to project
cd /home/ubuntu/legasee-quanta

# Build enhanced demo
make demo_enhanced

# Run it!
./demo_enhanced
```

---

## 🎮 Try These Cool Things

### 1. **Mouse Painting** 🖱️
- Move your mouse around the screen
- Click anywhere to spawn particle bursts
- Create patterns by clicking multiple locations

### 2. **Force Field Fun** ⚡
- Press `A` to add an attractor (pulls particles in)
- Press `R` to add a repeller (pushes particles away)
- Press `V` to add a vortex (makes particles spin)
- Watch particles interact with the fields!

### 3. **Visual Effects** 🌈
- Press `T` to toggle particle trails (motion blur)
- Press `F` to show/hide force field markers
- Watch the beautiful color gradients based on speed

### 4. **Experiment** 🧪
- Click rapidly to create particle clouds
- Add multiple force fields (A, R, V)
- Combine attractors and repellers for chaos
- Press `C` to clear and start fresh

---

## 🎯 Quick Controls Reference

| Key | Action |
|-----|--------|
| **Mouse Click** | Spawn particles |
| **A** | Add attractor ⊕ |
| **R** | Add repeller ⊖ |
| **V** | Add vortex ⊗ |
| **T** | Toggle trails |
| **F** | Toggle field markers |
| **P** | Pause/Resume |
| **C** | Clear particles |
| **X** | Clear force fields |
| **Q** | Quit |

---

## 🌟 What's Different?

### Before:
- Basic ASCII characters (`.`, `o`, `O`)
- Simple colors
- Keyboard only
- No trails
- No interactive forces

### After:
- Unicode graphics (`·`, `•`, `●`, `→`, `↑`)
- True 24-bit RGB colors
- Mouse support
- Particle trails with motion blur
- Interactive force fields (attractors, repellers, vortex)

---

## 💡 Cool Demos to Try

### 1. **Fountain Effect**
```
1. Clear everything: Press C, then X
2. Click at the bottom center
3. Add attractor at top: Press A
4. Watch particles fountain upward!
```

### 2. **Vortex Storm**
```
1. Clear: C, X
2. Add vortex in center: Press V
3. Click around the edges
4. Watch particles spiral!
```

### 3. **Particle Painting**
```
1. Enable trails: Press T
2. Click-drag with mouse (rapid clicks)
3. Create beautiful flowing patterns
```

### 4. **Chaos Mode**
```
1. Add attractor: Press A
2. Add repeller: Press R
3. Add vortex: Press V
4. Click everywhere!
5. Watch the chaos unfold
```

---

## 🔧 Terminal Requirements

**Works best with**:
- iTerm2 (macOS)
- Kitty
- Alacritty
- GNOME Terminal (recent versions)
- Windows Terminal
- Any terminal with UTF-8 + true color + mouse support

**Check your terminal**:
```bash
# UTF-8 support
locale | grep UTF-8

# True color support
echo $COLORTERM
# Should show: truecolor or 24bit
```

---

## 📊 Performance Tips

- **Smooth 60 FPS** with up to 1000 particles
- If laggy: Press `T` to disable trails
- Clear old particles: Press `C`
- Limit force fields to 3-5 for best performance

---

## 🎨 Visual Guide

### Particle Speeds:
```
·  = Very slow (blue)
•  = Slow (cyan)
●  = Medium (green/yellow)
⬤  = Fast (orange)
→  = Very fast (red, shows direction)
```

### Force Fields:
```
⊕  = Attractor (pulls in)
⊖  = Repeller (pushes away)
⊗  = Vortex (circular motion)
```

---

## 🚀 Next Steps

1. **Try the demos above**
2. **Experiment with combinations**
3. **Read `ENHANCED_FEATURES.md` for details**
4. **Check `IMPROVEMENT_RECOMMENDATIONS.md` for future ideas**

---

## ❓ Quick Troubleshooting

**Unicode characters look weird?**
- Run: `export LANG=en_US.UTF-8`

**No colors?**
- Your terminal may not support true color
- Still works, just less colorful!

**Mouse doesn't work?**
- Use keyboard controls instead (A, R, V)
- Check terminal mouse settings

**Too slow?**
- Press T to disable trails
- Press X to clear force fields
- Press C to clear particles

---

## 🎉 Have Fun!

Your particle simulator is now **10x cooler**! 

Enjoy creating beautiful particle effects! 🌈✨

---

*Quick Start Guide - October 16, 2025*

