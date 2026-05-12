# 2dmultiplayergame

## Dev Environment Setup
I personally run an Arch (btw) Linux environment.

So that is what the following guide will use.

```bash
sudo pacman -S --needed base-devel sdl3 sdl3_image sdl3_ttf sdl3_mixer
```

## Build the Game
I use CMake to build the game.
```bash
# Set CMake source and build directory
cmake -S . -B build
# Build the Project
cmake --build build
# Run The Game
./build/game
```
