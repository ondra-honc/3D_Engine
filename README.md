# 3D OpenGL Engine & Editor

A lightweight, from-scratch 3D engine built with **C++**, **OpenGL 3.3**, and **SDL2**. This project features a custom math library, real-time camera controls, and an integrated editor UI for object manipulation.

## 🚀 Features

* **Custom Math Library**: Hand-written `Vec3`, `Vec4`, and `mat4` implementations for transformations and projections.
* **Real-time Editor**: Powered by **ImGui**, allowing for live property editing (Position, Scale, Color).
* **Raycast Selection**: Pixel-perfect object selection using AABB (Axis-Aligned Bounding Box) intersection.
* **Multi-Select System**: Robust ID-based selection system supporting `CTRL + Click` for batch operations.
* **Dual Mode Control**:
    * **Editor Mode**: Free mouse for UI interaction and object selection.
    * **Camera Mode**: First-person style "Fly-cam" with WASD + Mouse look.

## 🛠️ Tech Stack

- **Graphics**: OpenGL 3.3 (Core Profile)
- **Windowing/Input**: SDL2
- **UI**: Dear ImGui
- **Loading**: GLAD

## 🎮 Controls

| Key | Action |
| :--- | :--- |
| **TAB** | Toggle between Editor and Camera mode |
| **W, A, S, D** | Move Camera (Camera Mode) |
| **SPACE / L-CTRL** | Fly Up / Fly Down (Camera Mode) |
| **Left Click** | Select Object |
| **CTRL + Left Click** | Multi-select Objects |
| **ESCAPE** | Exit Application |

## 🏗️ Architecture Note

The engine uses a stable **ID System** for object management. Unlike standard pointer-based selection, this ensures that even when the underlying memory reallocates (via `std::vector`), selection handles remain valid and crash-free.

## ⚙️ Building

1. Ensure you have **SDL2** and **OpenGL** development headers installed.
2. Include the following in your build path:
    - `glad.c`
    - `imgui/*.cpp`
3. Link against `SDL2`, `SDL2main`, and `OpenGL32`.