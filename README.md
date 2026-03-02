# OpenGL Practice (GLFW + GLAD)

A small OpenGL learning project written in C++.

Right now it renders a single triangle and supports toggling wireframe mode.

## Features
- Render a triangle
- Per-vertex color (from vertex position)
- Wireframe toggle

## Controls
- **W**: wireframe ON
- **E**: wireframe OFF

## Tech Stack
- C++
- OpenGL (GLSL **330 core**)
- GLFW (window/input)
- GLAD (OpenGL loader)
- CMake

## Build (Windows)

### Option A: Visual Studio (MSVC)
```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
