# GPGX Wide

This is a fork of the Genesis Plus GX emulator (https://github.com/ekeeke/Genesis-Plus-GX) that is meant to run classic Sonic games (and possibly other titles, with ROM modifications) in 16:9 widescreen. Specifically, it's meant to run with [this hack of Sonic 2](https://github.com/heyjoeway/s2disasm).

This project has strayed pretty far from base repository. It has a complete restructuring of the file layout, only supports building standalone emulators (with a completely rewritten makefile), has custom backends for input, sound, and rendering, and has a plethora of new features. Those include:
- Widescreen support
- Support for OGG music (custom Sonic 2 mod only)
- Multi-system multiplayer (to avoid dealing with the weird splitscreen VDP mode)
- Compilation for strange platforms, including
    - Emscripten
    - Nintendo Switch
- JSON configuration

## Building

### Windows

Compilation is done with the GNU Make system under [msys2](https://www.msys2.org/). You'll need to install some dependencies:
```
pacman -Syu # Re-run this until no new packages are installed
pacman -S git base-devel
pacman -S mingw-w64-x86_64-binutils mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-jansson
pacman -S mingw-w64-i686-binutils   mingw-w64-i686-gcc   mingw-w64-i686-SDL2   mingw-w64-i686-SDL2_image   mingw-w64-i686-SDL2_mixer   mingw-w64-i686-jansson # Optional
```

Once you've got them, it's pretty simple to compile:
```
git clone --recurse-submodules https://github.com/heyjoeway/Genesis-Plus-GX
cd Genesis-Plus-GX
make
```
Executable should end up at `./bin/Windows/gen.exe`.

### Emscripten

(need to finish this section)

You'll need to set up an [Emscripten development environment](https://emscripten.org/docs/getting_started/downloads.html#installation-instructions). Once you've got it ready:
```
git clone --recurse-submodules https://github.com/heyjoeway/Genesis-Plus-GX
cd Genesis-Plus-GX
emmake make
```
This will output files at `./bin/Emscripten`. You can serve these files locally to test them. Additionally, copy the files from `./emscripten` to the output folder if you want a more refined UI.