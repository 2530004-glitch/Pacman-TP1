# Pac-Man TP1

This project is a C++17 Pac-Man implementation built with raylib.

## Build

The primary build path is `nob`.

```powershell
.\setup.bat
cc -ggdb3 nob.c -o nob.exe
.\nob.exe
```

## Run

Launch `Deployment/game.exe` after building.

## Notes

- The maze is parsed at runtime from `assets/map.txt`.
- The game expects the root `assets/` folder to contain the required sprites.
