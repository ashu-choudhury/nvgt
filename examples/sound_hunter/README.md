# Sound Hunter - NVGT WebAssembly Example Game

An accessible, spatial audio game written in NVGT demonstrating the new **WebAssembly export** features.

---

## Game Overview

In **Sound Hunter**, an invisible sound source is hidden on a 1D grid (-10 to +10).
- A periodic sonar ping emanates from the target location with real-time stereo panning and distance-based volume attenuation.
- Use your ears (or screen reader feedback) to guide yourself toward the sound.
- Once you reach the target position, hit **Space** or **Enter** to collect it!
- Find 5 items to win!

---

## Controls

| Key | Action |
| --- | --- |
| **Left Arrow** or **A** | Move Left |
| **Right Arrow** or **D** | Move Right |
| **Space** or **Enter** | Collect Item |
| **Up Arrow** or **C** | Announce current position and score |
| **Escape** | Exit game |

---

## Quick Start: Testing in Your Browser

1. Run the included local server:
   ```bash
   python serve.py
   ```
2. Your browser will automatically open to `http://localhost:8000`.
3. Press **Enter**, **Space**, or click the **"Press Enter, Space, or Click to Play"** button to start!
4. If you use a screen reader (NVDA, JAWS, VoiceOver, Narrator, Orca), it will automatically read game status via the built-in ARIA live region.

---

## Compiling for Web

To recompile or package the game yourself:
```bash
nvgt -c -p web sound_hunter.nvgt -s "build.product_name=Sound Hunter"
```

This generates:
- **`sound_hunter_web/`**: A self-contained web directory ready to upload to any static host (GitHub Pages, Netlify, Apache, Nginx).
- **`sound_hunter_web.zip`**: A ready-to-upload zip file for **itch.io** (select *HTML* project type).

---

## Compiling for Other Platforms

NVGT supports cross-compiling the exact same script for any supported platform:

```bash
# Windows
nvgt -c -p windows sound_hunter.nvgt

# Linux
nvgt -c -p linux sound_hunter.nvgt

# macOS
nvgt -c -p mac sound_hunter.nvgt

# Android (APK)
nvgt -c -p android sound_hunter.nvgt
```
