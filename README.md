# Mesh Viewer (OpenGL 3.3 + SDL2)

Cross-platform CMake build (Windows, Linux, macOS) with modern OpenGL rendering/picking and SDL2 window/input layer.

## Build Requirements

- CMake 3.20+
- C++17 compiler
- OpenGL
- Git (for `FetchContent` dependencies)
- Internet access during first configure

## Build

Same commands on Windows, Linux, and macOS:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

The executable is named `meshviewer`.
