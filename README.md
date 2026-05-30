# 2dmultiplayergame

## Dev Environment Setup
I personally run an Arch (btw) Linux environment.

So that is what the following guide will use.

```bash
sudo pacman -S raylib cmake base-devel
```

## Build the Game
I use CMake to build the game.
```bash
# Set CMake source and build
cmake -B build & cmake --build build
# Run The Game
./build/main
```
