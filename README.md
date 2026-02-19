# 🔥 GPU Path-Traced Cinematic Urban Destruction (OpenGL + Compute Shaders)

<p align="center">
  <img src="banner.png" alt="Project Banner" width="100%"/>
</p>

> **What this is:** a cinematic urban destruction scene renderer that combines a **real-time raster pipeline** (interactive preview) with a **GPU compute-shader path tracer** (frame capture to video).  
> **Key features:** BVH acceleration, multi-bounce Monte Carlo path tracing, HDR tone mapping, scripted walk camera + timed earthquake/collapse, and a geometry processing demo (Laplacian / Taubin / curvature visualization).

---


## 🎮 How to use the project (3 “modes” + what to press)

### 1) Real-time interactive preview (default)
This is the **rasterized** renderer (OpenGL pipeline) for navigation and scene preview.

**What you do:**
1. Launch the executable.
2. Press **`M`** to enable mouse-look (FPS camera).
3. Move around with **WASD** (and optionally **Shift** for speed).
4. When you want to quit, press **Esc**.

### 2) Geometry demo (separate window)
This opens a **second window** that shows the collapse building mesh in 4 views:
- Original (solid shaded)
- Laplacian smoothing (wire overlay)
- Taubin smoothing (wire overlay)
- Curvature visualization (wire overlay)

**What you do:**
1. Press **`G`** to toggle the Geometry Demo window on/off.
2. In the Geometry Demo window:
   - Drag with **Left Mouse Button** to rotate.
   - Use the **Mouse Wheel** to zoom.

> Note: while the Geometry Demo window is open, the main rendering loop is effectively paused, so the main scene doesn’t keep updating in the background.

### 3) Ray-traced recording → frames → mp4 video
This is the **compute shader path tracer** (`shaders/rt_pt.comp`) running while recording is enabled.  
It captures frames to `frames/frame_XXXX.png` and then runs **ffmpeg** to generate `city_pathtrace.mp4`.

**What you do:**
1. From the main window, press **`R`** once to start recording.
2. The camera follows the scripted walk path.
3. When the camera passes **Z < -30**:
   - The event timer starts.
   - After **2 seconds** → earthquake shake begins.
   - After **4 seconds** → collapse begins + dust burst starts.
4. Recording stops automatically after the collapse has been seen and ~15 seconds have elapsed, then ffmpeg is called to create:
   - `city_pathtrace.mp4`

**Important requirements for recording:**
- You must have **ffmpeg** installed and available in your PATH.
- Create a folder named **`frames/`** next to the executable before recording:
  - `mkdir frames`

---

## ⌨️ Hotkeys & Controls (from `main.cpp`)

### Main window controls

#### Camera movement
| Key | Action |
|---|---|
| `W` `A` `S` `D` | Move forward / left / back / right |
| `Q` / `E` | Move down / up |
| `Left Shift` (hold) | Faster movement (speed boost) |
| Mouse | Look around (only when mouse-look is enabled) |

#### Mouse-look
| Key | Action |
|---|---|
| `M` | Enable mouse-look + lock cursor (FPS-style) |
| `N` | Disable mouse-look + release cursor |

#### Cinematic camera toggle (non-recording)
| Key | Action |
|---|---|
| `C` | Enable spline-based cinematic camera mode |
| `V` | Disable cinematic camera mode (back to free camera) |

#### Recording (ray-traced)
| Key | Action |
|---|---|
| `R` | Start ray-traced recording (frame capture + ffmpeg at the end) |

#### Geometry demo
| Key | Action |
|---|---|
| `G` | Toggle Geometry Demo window |

#### Quit
| Key | Action |
|---|---|
| `Esc` | Close the application |

---

### Geometry demo controls (inside the Geometry Demo window)

#### Navigation
| Input | Action |
|---|---|
| Left mouse drag | Orbit/rotate view |
| Mouse wheel | Zoom in/out |

---

## 🧱 Destruction event timeline (how it’s synchronized)

The destruction is driven by a **spatial trigger** + **timed delays**:

- **Trigger:** when `cam.pos.z < -30.0f`, the event timer starts.
- **After 2.0 seconds:** earthquake camera shake begins.
- **After 4.0 seconds:** the building collapse begins (physics chunks are activated).
- A dust burst is triggered for ~10 seconds to increase particles during the collapse.

This is synchronized with the recording mode so the captured ray-traced sequence always includes the full event.

---

## 🛠 Build & Run

### Dependencies
This project uses:
- OpenGL 4.3+ (compute shaders)
- GLFW + GLAD
- GLM
- stb_image_write / stb_easy_font
- Bullet physics (via your project `dep/` / linked setup)
- ffmpeg (only needed for the recording → mp4 step)

### Build (CMake)
Typical flow:

```bash
mkdir build
cd build
cmake ..
cmake --build . -j
```

### Run
Run the built executable from a directory that has access to your `assets/` and `shaders/` folders.

For ray-traced recording:
```bash
mkdir -p frames
```

---

## 📁 Output files

During recording:
- `frames/frame_0000.png`, `frames/frame_0001.png`, ...

After recording ends:
- `city_pathtrace.mp4`

---

## 📌 Notes / troubleshooting

- **If mouse look doesn’t work:** press `M` (it is disabled by default).
- **If video is not created:** ensure `ffmpeg` is installed and callable from terminal (`ffmpeg -version`).
- **If it can’t write frames:** make sure `frames/` exists next to the executable.
- **If textures look wrong:** the code forces `GL_REPEAT` wrapping on mesh albedo textures.

---

## 👤 Author
Hadi Jaber — Télécom Paris
