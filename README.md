# OpenGL Practice (GLFW + GLAD)

A small OpenGL learning project written in C++ (GLFW + GLAD).  
This repo tracks my progress through core OpenGL pipeline concepts: VAO/VBO, shader programs, vertex attributes, uniforms, wireframe rendering, alpha blending, depth testing, textures, and viewport resize handling.

## Demo / What it does now
- Draws multiple triangles (single triangles + connected triangles)
- Uses multiple shader programs (default / yellow / light-blue overlay / vertex-color / textured)
- Demonstrates interleaved VBO layout (position + color)
- Uses uniforms for time-based animation (“breathing”) and per-draw offsets
- Renders a textured quad (UV + sampler2D) using stb_image
- Supports wireframe toggle and layered rendering (filled base + wireframe overlay)
- Handles window resize via framebuffer callback + glViewport
- Uses depth testing to control layering

## Features

### Rendering
- `glDrawArrays` with flexible vertex counts (3 and 6)
- Connected triangles (shared edge, rendered as 6 vertices)
- Multiple VAO/VBO configurations for different geometry
- Layered rendering for a “tip-to-tip” shape:
  - base layer stays filled
  - overlay layer can switch to wireframe (optionally thicker line)

### Shaders (GLSL 330 core)
- Default program (color derived from vertex position)
- Yellow fragment shader program
- Light-blue overlay program (alpha / overlay usage)
- Vertex-color program (attribute location 0/1 + uniform control)
- Texture program (attribute location 0/2 UV + sampler2D)

### Vertex Attributes & Data Layout
- Interleaved vertex buffer example: **[x y z r g b]** per vertex
- Attribute mapping:
  - `location = 0`: position (`vec3`)
  - `location = 1`: color (`vec3`)
  - `location = 2`: UV (`vec2`) for textured quad
- Correct stride/offset setup using `glVertexAttribPointer`

### Uniforms (CPU → GPU)
- `ourColor` (vec4): time-based “breathing” + key toggle
- `uOffset` (vec2): per-draw offset for positioning

### Depth & Blending
- Depth test enabled: `glEnable(GL_DEPTH_TEST)` + `glClear(... | GL_DEPTH_BUFFER_BIT)`
- Alpha blending enabled:
  - `glEnable(GL_BLEND)`
  - `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`

### Textures
- Loads an image with `stb_image.h`
- Uploads to GPU with `glTexImage2D` + `glGenerateMipmap`
- Samples in fragment shader via `sampler2D` + `texture()`

## Controls
- **W**: wireframe ON (global)
- **E**: wireframe OFF (global)
- **C**: toggle uniform color mode (e.g., breathing vs fixed)

## Build (Windows + CMake)

### Option A: Visual Studio (MSVC)
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Option B: MinGW (Git Bash / MINGW64)
```bash
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

## Assets / Texture Path Notes
Texture files live in:
- `texture/blackice.jpg`

When running from Visual Studio, the working directory can be inside `out/build/...`,
so relative paths like `texture/blackice.jpg` may fail if the texture folder is not next to the executable.

If the texture fails to load:
- Print the current working directory (CWD)
- Adjust the relative path (e.g., `../../../../texture/blackice.jpg`)
- Or copy the `texture/` folder next to the executable (recommended)

(Optional) Post-build copy via CMake (avoids path issues):
```cmake
# Replace "hello" with your actual target name
add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/texture
            $<TARGET_FILE_DIR:hello>/texture
)
```

## Project Structure
- `CMakeProject1/CMakeProject1/main.cpp` — main playground
- `CMakeProject1/CMakeProject1/stb_image.h` — stb_image (single-header image loader)
- `texture/` — texture asset(s)

## Roadmap / Next Steps
- [ ] Move shaders into separate `.vs` / `.fs` files and load from disk
- [ ] Wrap VAO/VBO/Texture/Shader into small helper classes (Mesh / Shader)
- [ ] Add MVP transforms (model/view/projection) + camera
- [ ] Try `glDrawElements` with EBO for indexed meshes