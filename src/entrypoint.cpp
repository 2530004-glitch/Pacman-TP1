#include "entrypoint.h"
#include "raylib.h"
#include <time.h>

// ─── Grid Configuration ───────────────────────────────────────────────────────
#define CELL_SIZE   64      // Pixel size of each grid cell (width & height)
#define GRID_COLS   14      // Number of columns in the grid
#define GRID_ROWS   14      // Number of rows in the grid
#define WINDOW_W    (GRID_COLS * CELL_SIZE)  // Total window width  (896px)
#define WINDOW_H    (GRID_ROWS * CELL_SIZE)  // Total window height (896px)

// ─── Grid State ───────────────────────────────────────────────────────────────
// Each cell is either a wall (true) or empty (false).
// Randomly initialized at startup.
bool isWall[GRID_ROWS][GRID_COLS] = {0};

// ─── Entry Point ──────────────────────────────────────────────────────────────
// TODO: Replace this function body with your Engine class.
//       Create an Engine instance and call engine.start() here instead.
void raylib_start(void)
{
    // ── Initialization ────────────────────────────────────────────────────────
    InitWindow(WINDOW_W, WINDOW_H, "Pacman - Grid Test");
    SetRandomSeed(time(NULL));

    // Randomly mark each cell as a wall or open path
    for (int row = 0; row < GRID_ROWS; ++row)
        for (int col = 0; col < GRID_COLS; ++col)
            isWall[row][col] = GetRandomValue(0, 1);

    // ── Main Game Loop ────────────────────────────────────────────────────────
    while (!WindowShouldClose())
    {
        BeginDrawing();
        ClearBackground(BLACK);

        // ── Draw Grid ─────────────────────────────────────────────────────────
        // Even rows are offset to the right, odd rows are not.
        // Even columns → RED outline, odd columns → BLUE outline.
        // This creates a checkerboard-like staggered visual.
        for (int row = 0; row < GRID_ROWS; ++row)
        {
            // Offset shifts the row by one cell-width if the row index is even
            float rowOffset = (row % 2 == 0) ? (float)CELL_SIZE : 0.0f;

            for (int col = 0; col < GRID_COLS; ++col)
            {
                // Even columns shift right by the row offset,
                // odd columns shift left by it — creating the stagger
                float cellX = (col % 2 == 0)
                    ? (float)(col * CELL_SIZE + rowOffset)
                    : (float)(col * CELL_SIZE - rowOffset);

                float cellY = (float)(row * CELL_SIZE);

                Rectangle cell = { cellX, cellY, CELL_SIZE, CELL_SIZE };
                Color outlineColor = (col % 2 == 0) ? RED : BLUE;

                // Draw the cell border (wall outline)
                DrawRectangleLinesEx(cell, 4, outlineColor);

                // ── Draw Wall Fill ────────────────────────────────────────────
                // If this cell is marked as a wall, fill it with white
                // NOTE: Wall is drawn at the raw grid position (no stagger offset)
                // TODO: Replace with a proper wall sprite / texture
                if (isWall[row][col])
                    DrawRectangle(col * CELL_SIZE, row * CELL_SIZE,
                                  CELL_SIZE, CELL_SIZE, WHITE);
            }
        }

        // TODO: Draw player here using player position
        // Example: DrawTexture(playerSprite, playerX, playerY, WHITE);

        EndDrawing();
    }

    // ── Cleanup ───────────────────────────────────────────────────────────────
    CloseWindow();
}