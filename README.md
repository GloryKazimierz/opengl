# OpenGL Practice (GLFW + GLAD)

A small OpenGL learning project written in C++ (GLFW + GLAD).  
This repo tracks my progress through core OpenGL pipeline concepts: VAO/VBO, shader programs, vertex attributes, uniforms, wireframe rendering, alpha blending, and viewport resize handling.

## Demo / What it does now
- Draws multiple triangles (single triangles + two connected triangles)
- Shows different shader outputs (default, yellow, light-blue with alpha)
- Demonstrates per-vertex color using an interleaved VBO layout (position + color)
- Uses uniforms to animate color over time (“breathing”) and to apply per-draw offsets
- Supports wireframe mode toggle

## Features
### Rendering
- `glDrawArrays` with flexible vertex counts (3 and 6)
- Two connected triangles (shared edge, rendered as 6 vertices)
- Two triangles drawn using different VAO/VBO
- Special layered rendering for the “tip-to-tip” shape:
  - base layer stays filled
  - overlay layer can switch to wireframe

### Shaders
- Multiple shader programs:
  - Default program (color derived from vertex position)
  - Yellow fragment shader program
  - Light-blue fragment shader program (uses alpha)
  - Vertex-color program (attribute location 0/1 + uniform control)
- GLSL 330 core

### Vertex Attributes & Data Layout
- Interleaved vertex buffer for one triangle:  
  **[x y z r g b]** per vertex
- Attribute mapping:
  - `location = 0`: position (`vec3`)
  - `location = 1`: color (`vec3`)
- Correct stride/offset setup using `glVertexAttribPointer`

### Uniforms (CPU → GPU control)
- `ourColor` (vec4): used for time-based color “breathing” and key toggles
- `uOffset` (vec2): used to offset geometry per draw

### Window / Viewport
- `framebuffer_size_callback` updates `glViewport` so rendering scales correctly when resizing the window

### Alpha Blending
- Enabled blending:
  - `glEnable(GL_BLEND)`
  - `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`
- Note: alpha blending is visible when there is something drawn behind the transparent object (or a non-black background)

## Controls
- **W**: wireframe ON (global)
- **E**: wireframe OFF (global)
- **C**: toggle uniform color mode (e.g., breathing vs fixed)

## Build (Windows + CMake)
> Works with either MSVC (Visual Studio) or MinGW.

### Option A: Visual Studio (MSVC)
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
