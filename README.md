# OpenGL Practice (GLFW + GLAD)

A small OpenGL learning project written in C++ using GLFW + GLAD.

This repository tracks my progress through core OpenGL and real-time rendering concepts, including VAO/VBO, shader programs, vertex attributes, uniforms, wireframe rendering, alpha blending, depth testing, textures, and viewport resize handling.

The project also includes an experimental local AI assistant that uses **Ollama + Qwen3.5** to translate natural-language instructions into validated renderer commands.

The AI module is intentionally separated from the rendering core.  
The LLM does **not** call OpenGL directly — it only produces high-level validated commands, while the C++ renderer decides how those commands are executed.

---

## Demo / What it does now

- Draws multiple triangles (single triangles + connected triangles)
- Uses multiple shader programs (default / yellow / light-blue overlay / vertex-color / textured)
- Demonstrates interleaved VBO layouts (position + color)
- Uses uniforms for time-based animation ("breathing") and per-draw offsets
- Renders a textured quad (UV + `sampler2D`) using `stb_image`
- Supports wireframe toggle and layered rendering (filled base + wireframe overlay)
- Handles window resize through framebuffer callback + `glViewport`
- Uses depth testing to control layering
- Uses alpha blending
- Supports natural-language renderer control through a local LLM assistant
- Includes a small Tkinter GUI for sending AI renderer instructions

---

## Features

### Rendering

- `glDrawArrays` with flexible vertex counts (3 and 6)
- Connected triangles (shared edge, rendered as 6 vertices)
- Multiple VAO/VBO configurations for different geometry
- Layered rendering for a "tip-to-tip" shape:
  - base layer stays filled
  - overlay layer can switch to wireframe
  - wireframe overlay can use a thicker line width
- Runtime background color control through an external renderer command bridge

### Shaders (GLSL 330 core)

- Default program (color derived from vertex position)
- Yellow fragment shader program
- Light-blue overlay program (alpha / overlay usage)
- Vertex-color program (attribute location 0/1 + uniform control)
- Texture program (attribute location 0/2 UV + `sampler2D`)

### Vertex Attributes & Data Layout

- Interleaved vertex buffer example: **[x y z r g b]** per vertex
- Attribute mapping:
  - `location = 0`: position (`vec3`)
  - `location = 1`: color (`vec3`)
  - `location = 2`: UV (`vec2`) for textured quad
- Correct stride/offset setup using `glVertexAttribPointer`

### Uniforms (CPU → GPU)

- `ourColor` (`vec4`): time-based "breathing" + key toggle
- `uOffset` (`vec2`): per-draw offset for positioning

### Depth & Blending

- Depth test enabled:

```cpp
glEnable(GL_DEPTH_TEST);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

- Alpha blending enabled:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

### Textures

- Loads an image with `stb_image.h`
- Uploads texture data to the GPU using:
  - `glTexImage2D`
  - `glGenerateMipmap`
- Samples textures in the fragment shader using:
  - `sampler2D`
  - `texture()`

---

# AI Renderer Assistant

The repository contains an experimental AI module in:

```text
ai_module/
```

The goal of this module is to explore how a local LLM can interact with a graphics renderer without allowing the model to directly control OpenGL.

The current architecture is:

```text
Natural-language instruction
        ↓
Tkinter GUI / CLI
        ↓
Ollama
        ↓
Qwen3.5 4B
        ↓
Structured renderer command
        ↓
Command validation
        ↓
Python bridge
        ↓
renderer_command.txt
        ↓
C++ renderer
        ↓
OpenGL
```

For example, the user can enter:

```text
give me a cold dark blue background
```

Qwen may interpret it as:

```json
{
  "command": "set_background",
  "color": [0.1, 0.2, 0.4]
}
```

The Python bridge converts the validated command into:

```text
set_background 0.1 0.2 0.4
```

The C++ renderer reads the command and applies it using its own OpenGL code.

For example:

```cpp
glClearColor(
    backgroundR,
    backgroundG,
    backgroundB,
    1.0f
);
```

The AI therefore decides **what the user wants**, while the renderer remains responsible for **how the graphics operation is performed**.

---

## AI Module Design

The AI code is intentionally isolated from the OpenGL renderer.

```text
Renderer Core               AI Module
-------------               ---------
C++                         Python
OpenGL                      Ollama
GLFW                        Qwen3.5
Shaders                     Command validation
VAO / VBO                   Tkinter GUI
Textures                    Command bridge

        ↑                       ↓
        └── Renderer Command ───┘
```

The LLM never calls functions such as:

```cpp
glClearColor(...)
glPolygonMode(...)
glDrawArrays(...)
glBindBuffer(...)
```

Instead, it is restricted to high-level renderer commands such as:

```text
set_background
set_wireframe
reset_scene
```

Only commands that pass validation can be forwarded toward the renderer.

---

## AI Parsers

The AI module currently contains multiple parser backends.

### Rule-based parser

A simple deterministic parser used during early development.

Example:

```text
change the background to blue
```

becomes:

```json
{
  "command": "set_background",
  "color": [0.0, 0.0, 1.0]
}
```

### Local Ollama parser

The main experimental AI backend currently uses:

```text
Qwen3.5 4B
```

running locally through Ollama.

This allows more flexible instructions such as:

```text
make the background feel like midnight
```

or:

```text
give me a cold dark blue background
```

without requiring exact hard-coded phrases.

The local Ollama workflow does not require a paid API.

### Optional API parser

An additional LLM parser backend is also kept in the AI module for experimentation with remote APIs.

The local Ollama backend is currently preferred for development.

---

## Running the AI Assistant

### 1. Install Ollama

Install Ollama for your platform.

Then download/run the local model:

```powershell
ollama run qwen3.5:4b
```

After the model is installed, Ollama exposes a local server that the AI module can use.

---

### 2. Start the OpenGL renderer

Build and launch the renderer normally.

The renderer watches for validated commands produced by the AI bridge.

---

### 3. Launch the AI GUI

From the repository root:

```powershell
python -B ai_module/gui.py
```

This opens a small Tkinter control window.

You can enter instructions such as:

```text
give me a cold dark blue background
```

The instruction is:

1. sent to the local Qwen model
2. converted into a structured command
3. validated
4. sent through the bridge
5. consumed by the C++ renderer

---

## AI Command-Line Interface

The GUI is optional.

The same system can also be tested directly from PowerShell:

```powershell
python -B ai_module/main.py --parser ollama --bridge "give me a cold dark blue background"
```

Example output:

```json
{
  "command": "set_background",
  "color": [0.1, 0.2, 0.4]
}
```

The bridge then sends the command to the running renderer.

Without `--bridge`, the program only prints the structured JSON result.

Example:

```powershell
python -B ai_module/main.py --parser ollama "make the background feel like midnight"
```

---

## Current AI Bridge Support

Currently implemented:

- Natural-language parsing
- Local Qwen3.5 inference through Ollama
- Structured renderer commands
- Strict command validation
- Python → C++ command bridge
- Atomic bridge-file writes
- Tkinter GUI
- Background color control

Currently being expanded:

- Wireframe ON/OFF
- Scene reset
- Additional renderer state controls

---

## Controls

### Keyboard

- **W**: wireframe ON (global)
- **E**: wireframe OFF (global)
- **C**: toggle uniform color mode (e.g. breathing vs fixed)

### AI Assistant

Current example:

```text
give me a cold dark blue background
```

which can update the renderer background through the local AI pipeline.

---

## Build (Windows + CMake)

### Option A: Visual Studio (MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Option B: MinGW (Git Bash / MINGW64)

```powershell
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build
```

---

## Assets / Texture Path Notes

Texture files live in:

```text
texture/blackice.jpg
```

When running from Visual Studio, the working directory can be inside `out/build/...`, so relative paths such as:

```text
texture/blackice.jpg
```

may fail if the texture folder is not next to the executable.

If the texture fails to load:

- Print the current working directory (CWD)
- Adjust the relative path (for example `../../../../texture/blackice.jpg`)
- Or copy the `texture/` folder next to the executable

Optional post-build copy through CMake:

```cmake
# Replace "hello" with your actual target name

add_custom_command(TARGET hello POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/texture
            $<TARGET_FILE_DIR:hello>/texture
)
```

---

## Project Structure

```text
opengl/
│
├── CMakeProject1/
│   └── CMakeProject1/
│       ├── main.cpp
│       └── stb_image.h
│
├── texture/
│   └── blackice.jpg
│
└── ai_module/
    ├── commands.py
    ├── ollama_parser.py
    ├── llm_parser.py
    ├── bridge.py
    ├── main.py
    ├── gui.py
    └── README.md
```

### Main renderer files

- `CMakeProject1/CMakeProject1/main.cpp`
  - main OpenGL playground / renderer

- `CMakeProject1/CMakeProject1/stb_image.h`
  - single-header image loader

- `texture/`
  - texture assets

### AI module files

- `ai_module/commands.py`
  - command definitions and rule-based parsing

- `ai_module/ollama_parser.py`
  - local Ollama / Qwen instruction parser

- `ai_module/llm_parser.py`
  - optional remote LLM parser

- `ai_module/bridge.py`
  - validated command bridge between Python and the C++ renderer

- `ai_module/main.py`
  - command-line interface

- `ai_module/gui.py`
  - Tkinter AI control interface

- `ai_module/README.md`
  - detailed AI module documentation

---

## Design Principle

The renderer and the AI assistant are intentionally separated.

```text
AI:
"What does the user want?"

        ↓

Validated Renderer Command

        ↓

C++ Renderer:
"How should OpenGL perform it?"
```

This keeps the graphics implementation inside the renderer while allowing natural-language interaction to be explored as a separate system.

---

## Roadmap / Next Steps

### Graphics

- Move shaders into separate `.vs` / `.fs` files and load them from disk
- Wrap VAO/VBO/Texture/Shader into small helper classes (`Mesh`, `Shader`, etc.)
- Add MVP transforms (Model / View / Projection)
- Add a camera system
- Try `glDrawElements` with EBO for indexed meshes
- Add lighting
- Add model loading
- Continue improving renderer architecture

### AI / Renderer Integration

- Add AI-controlled wireframe ON/OFF
- Add scene reset commands
- Add camera-related AI commands
- Add more renderer commands as new graphics features are implemented
- Give the AI access to safe renderer/scene state
- Explore scene-aware natural-language commands
- Eventually integrate the AI assistant into an in-renderer editor UI
- Explore AI-assisted renderer debugging and visual feedback