# OpenGL-Game

## [Web Demo](https://marth1nus.github.io/OpenGL-Game)

## How to Compile

### Start but cloning the repo

```bash
git clone https://github.com/Marth1nus/OpenGL-Game
cd OpenGL-Game
```

The project uses **CMake Presets** to compile. \
There are 3 **CMake Presets** that can be built: `windows`, `web`, `native`, `default = native`. \
All presets are `Multi-Config` so adding `--config Debug|Release` to `cmake --build` will allow you to explicitly compile with optimization. **Default Config** is `Debug`.

### For a Windows-Platform build

```sh
cmake --preset windows
cmake --build --preset windows
```

This will build using the **visual studio generator** so it can be done on any windows environment.

### For a Web-Platform build

```sh
cmake --preset web
cmake --build --preset web
```

This will build using **Emscripten** for **WASM**.

**Note**: Make sure that the `EMSDK` environment variable is available as it is used to find the toolchain file. The preset is disabled if the variable is not present.

### For a Native-Unix-Platform build

```sh
cmake --preset native
cmake --build --preset native
```

**Note** this preset might work on **Windows** with **ninja** installed and in the **Developer PowerShell for VS 18** (ignored for testing).

## Support Goals

- **OpenGL API versions**:
  - OpenGL ES 3.2
  - WebGL2
- **Platforms**:
  - Windows
  - Linux (tested on wsl arch-linux)
  - Emscripten (web)
