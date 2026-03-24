/*
    Game.cpp
    --------
    This file implements the main Game class.
    It handles the main loop, menu screens, texture loading, and screen
    transitions. The GameManager handles the maze gameplay itself.
*/

#include "Game.h"

#include <cmath>
#include <cstdlib>
#include <string>

static const int SCREEN_WIDTH = 28 * GameManager::TileSize;
static const int SCREEN_HEIGHT = 31 * GameManager::TileSize + GameManager::HudHeight;
static const int MENU_OPTION_COUNT = 3;
static const int MENU_BUTTON_WIDTH = 340;
static const int MENU_BUTTON_HEIGHT = 58;
static const int MENU_FIRST_BUTTON_Y = 300;
static const int MENU_BUTTON_SPACING = 88;
static const float MENU_BLINK_TIME = 1.0f;
static const float MENU_BLINK_VISIBLE_TIME = 0.5f;
static const float END_SCREEN_TIME = 3.0f;

static const char* MENU_OPTIONS[MENU_OPTION_COUNT] = {
    "START GAME",
    "GAME RULES",
    "QUIT"
};

// Sets every field of a texture to zero.
// texture points to the texture struct that should be cleared.
static void ClearTextureData(Texture2D* texture)
{
    // Reset all texture fields so raylib treats this as unloaded.
    texture->id = 0;
    texture->width = 0;
    texture->height = 0;
    texture->mipmaps = 0;
    texture->format = 0;
}

// Draws a filled circle into a raylib Image.
// image is the image to edit, centerX and centerY are the circle center,
// radius is the circle radius, and color is the draw color.
static void DrawFilledCircle(Image* image, int centerX, int centerY, int radius, Color color)
{
    // Scan every pixel inside the square around the circle.
    for (int y = centerY - radius; y <= centerY + radius; y++)
    {
        for (int x = centerX - radius; x <= centerX + radius; x++)
        {
            // Measure how far this pixel is from the circle center.
            int deltaX = x - centerX;
            int deltaY = y - centerY;
            int distanceSquared = deltaX * deltaX + deltaY * deltaY;

            // Only color the pixel if it is inside the circle.
            if (distanceSquared <= radius * radius)
            {
                ImageDrawPixel(image, x, y, color);
            }
        }
    }
}

// Returns the correct asset path for both root and Deployment launches.
// path is the normal asset path relative to the project root.
static std::string ResolveAssetPath(const char* path)
{
    // First try the normal path from the project root.
    if (FileExists(path))
    {
        return path;
    }

    // Then try the parent folder for runs that start inside Deployment.
    std::string deploymentRelative = std::string("..\\") + path;
    if (FileExists(deploymentRelative.c_str()))
    {
        return deploymentRelative;
    }

    // If nothing matched, return the original path for the error message.
    return path;
}

// Builds a rectangle for a centered menu button.
// centerX is the horizontal center, topY is the top edge,
// width and height are the button size.
static Rectangle MakeButtonRectangle(int centerX, int topY, int width, int height)
{
    // Fill out the rectangle fields one by one for clarity.
    Rectangle button;
    button.x = (float)(centerX - width / 2);
    button.y = (float)topY;
    button.width = (float)width;
    button.height = (float)height;
    return button;
}

// Draws one menu button.
// label is the text to show, and selected decides the highlight colors.
static void DrawCenteredButton(int centerX, int topY, int width, int height, const char* label, bool selected)
{
    // Build the button rectangle first.
    Rectangle button = MakeButtonRectangle(centerX, topY, width, height);

    // Pick colors based on whether this option is selected.
    Color fillColor = Color{24, 35, 62, 255};
    Color borderColor = Color{84, 121, 214, 255};
    Color textColor = RAYWHITE;
    if (selected)
    {
        fillColor = Color{246, 214, 41, 255};
        borderColor = YELLOW;
        textColor = BLACK;
    }

    // Draw the button background, border, and centered label text.
    DrawRectangleRec(button, fillColor);
    DrawRectangleLinesEx(button, 3.0f, borderColor);
    DrawText(label, centerX - MeasureText(label, 28) / 2, topY + 14, 28, textColor);
}

// Creates the main Game object and fills it with default values.
Game::Game()
    : manager(*this)
{
    // Clear every texture so the unload code is safe.
    ClearTextureData(&pacManTexture);
    ClearTextureData(&coinTexture);
    ClearTextureData(&bigCoinTexture);
    ClearTextureData(&inkyTexture);
    ClearTextureData(&pinkyTexture);
    ClearTextureData(&clydeTexture);
    ClearTextureData(&frightenedTexture);
    ClearTextureData(&wallTexture);

    // Start with no frame time, the menu state, and no assets loaded.
    deltaTime = 0.0f;
    stateTimer = 0.0f;
    state = MainMenuState;
    menuSelection = 0;
    assetsLoaded = false;
}

// Cleans up the game when the object is destroyed.
Game::~Game()
{
    // Reuse the normal shutdown function.
    end();
}

// Loads one texture and stops the game if the file is missing.
// texture points to the destination texture, and path is the asset path.
void Game::LoadRequiredTexture(Texture2D* texture, const char* path)
{
    // Resolve the path first so Deployment runs still find the asset.
    std::string resolvedPath = ResolveAssetPath(path);

    // Ask raylib to load the texture from disk.
    *texture = LoadTexture(resolvedPath.c_str());

    // Stop immediately if the texture could not be loaded.
    if (texture->id == 0)
    {
        TraceLog(LOG_ERROR, "Failed to load texture: %s", resolvedPath.c_str());

        // Close the window first if it already exists.
        if (IsWindowReady())
        {
            CloseWindow();
        }

        // Exit so the game does not continue with missing assets.
        std::exit(1);
    }
}

// Loads the cherry texture, or builds a simple fallback cherry image.
// texture points to the destination texture.
void Game::LoadCherryTexture(Texture2D* texture)
{
    // Try to load a real cherry file first.
    std::string resolvedPath = ResolveAssetPath("assets/Cherry.png");
    if (FileExists(resolvedPath.c_str()))
    {
        *texture = LoadTexture(resolvedPath.c_str());
        return;
    }

    // Build a tiny cherry image in memory if the file does not exist.
    Image image = GenImageColor(16, 16, BLANK);
    DrawFilledCircle(&image, 5, 10, 4, RED);
    DrawFilledCircle(&image, 10, 10, 4, RED);

    // Draw the stem of the cherry.
    for (int y = 2; y <= 6; y++)
    {
        ImageDrawPixel(&image, 7, y, DARKGREEN);
    }

    // Draw the small angled branch pieces.
    for (int offset = 0; offset <= 3; offset++)
    {
        ImageDrawPixel(&image, 7 - offset, 5 - offset, DARKGREEN);
        ImageDrawPixel(&image, 7 + offset, 5 - offset, DARKGREEN);
    }

    // Draw a small leaf near the top.
    for (int x = 6; x <= 8; x++)
    {
        ImageDrawPixel(&image, x, 2, GREEN);
    }

    // Convert the temporary image into a texture, then free the image.
    *texture = LoadTextureFromImage(image);
    UnloadImage(image);
}

// Loads every texture needed by the current game.
void Game::LoadAssets()
{
    // Skip the work if the textures were already loaded.
    if (assetsLoaded)
    {
        return;
    }

    // Load every texture used by Pac-Man, the ghosts, pellets, and walls.
    LoadRequiredTexture(&pacManTexture, "assets/PacMan.png");
    LoadRequiredTexture(&coinTexture, "assets/Coin.png");
    LoadCherryTexture(&bigCoinTexture);
    LoadRequiredTexture(&inkyTexture, "assets/greenGhost.png");
    LoadRequiredTexture(&pinkyTexture, "assets/yellowGhost.png");
    LoadRequiredTexture(&clydeTexture, "assets/orangeGhost.png");
    LoadRequiredTexture(&frightenedTexture, "assets/blueGhost.png");
    LoadRequiredTexture(&wallTexture, "assets/Tileset.png");

    // Give the gameplay manager access to the loaded textures.
    manager.SetTextures(&pacManTexture, &coinTexture, &bigCoinTexture, &inkyTexture,
                        &pinkyTexture, &clydeTexture, &frightenedTexture, &wallTexture);

    // Remember that the asset step is complete.
    assetsLoaded = true;
}

// Unloads one texture safely if it is currently loaded.
// texture points to the texture that should be freed.
void Game::UnloadTextureIfLoaded(Texture2D* texture)
{
    // Only unload textures that raylib marked as valid.
    if (texture->id > 0)
    {
        UnloadTexture(*texture);
    }

    // Clear the struct so double unloads stay harmless.
    ClearTextureData(texture);
}

// Unloads every texture owned by the Game.
void Game::UnloadAssets()
{
    // Skip the work if nothing was loaded yet.
    if (!assetsLoaded)
    {
        return;
    }

    // Free every texture one by one.
    UnloadTextureIfLoaded(&pacManTexture);
    UnloadTextureIfLoaded(&coinTexture);
    UnloadTextureIfLoaded(&bigCoinTexture);
    UnloadTextureIfLoaded(&inkyTexture);
    UnloadTextureIfLoaded(&pinkyTexture);
    UnloadTextureIfLoaded(&clydeTexture);
    UnloadTextureIfLoaded(&frightenedTexture);
    UnloadTextureIfLoaded(&wallTexture);

    // Remember that the assets are no longer available.
    assetsLoaded = false;
}

// Draws the large title area on the main menu.
void Game::DrawMenuTitle() const
{
    // Draw the big game title first.
    const char* title = "PAC-MAN";
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, 64)) / 2, 70, 64, YELLOW);

    // Draw a short subtitle under the title.
    const char* subtitle = "Clear the maze, dodge ghosts, and hunt for the high score.";
    DrawText(subtitle, (SCREEN_WIDTH - MeasureText(subtitle, 24)) / 2, 155, 24, LIGHTGRAY);
}

// Draws the stack of three menu buttons.
void Game::DrawMenuButtons() const
{
    // Find the shared horizontal center for all buttons.
    int centerX = SCREEN_WIDTH / 2;

    // Draw each button with the selected state highlighted.
    for (int index = 0; index < MENU_OPTION_COUNT; index++)
    {
        int buttonTop = MENU_FIRST_BUTTON_Y + index * MENU_BUTTON_SPACING;
        bool selected = false;
        if (menuSelection == index)
        {
            selected = true;
        }

        DrawCenteredButton(centerX, buttonTop, MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT,
                           MENU_OPTIONS[index], selected);
    }
}

// Draws the menu control hints near the bottom of the screen.
void Game::DrawMenuHints() const
{
    // Explain how to move through the menu.
    const char* inputHint = "UP / DOWN  Select     ENTER  Confirm";
    DrawText(inputHint, (SCREEN_WIDTH - MeasureText(inputHint, 24)) / 2, 620, 24, RAYWHITE);

    // Explain how to close the game from the menu.
    const char* closeHint = "ESC closes the game";
    DrawText(closeHint, (SCREEN_WIDTH - MeasureText(closeHint, 22)) / 2, 664, 22, GRAY);

    // Blink a short hint so the menu feels alive.
    if (std::fmod(stateTimer, MENU_BLINK_TIME) < MENU_BLINK_VISIBLE_TIME)
    {
        const char* selectionHint = "The highlighted option is ready.";
        DrawText(selectionHint, (SCREEN_WIDTH - MeasureText(selectionHint, 22)) / 2, 730, 22, YELLOW);
    }
}

// Draws the complete main menu screen.
void Game::DrawMenu() const
{
    // Clear the screen before drawing the menu layers.
    ClearBackground(BLACK);

    // Draw the title, buttons, and hint text.
    DrawMenuTitle();
    DrawMenuButtons();
    DrawMenuHints();
}

// Draws the controls section on the rules screen.
void Game::DrawRulesControls() const
{
    // Show the heading first.
    DrawText("CONTROLS", 70, 150, 30, RAYWHITE);

    // Explain the movement inputs in simple lines.
    DrawText("Arrow keys move Pac-Man through the maze.", 70, 195, 24, RAYWHITE);
    DrawText("Queue a direction early and Pac-Man turns as soon as the path opens.", 70, 235, 24, RAYWHITE);
}

// Draws the objective section on the rules screen.
void Game::DrawRulesObjective() const
{
    // Show the heading first.
    DrawText("OBJECTIVE", 70, 320, 30, RAYWHITE);

    // Explain the main goal and the danger from ghosts.
    DrawText("Eat every pellet to clear the level.", 70, 365, 24, RAYWHITE);
    DrawText("Touching a ghost in chase mode costs one life.", 70, 405, 24, RAYWHITE);
    DrawText("Cherries turn ghosts blue for a short time.", 70, 445, 24, RAYWHITE);
    DrawText("Blue ghosts can be eaten for bonus points.", 70, 485, 24, RAYWHITE);
}

// Draws the scoring section on the rules screen.
void Game::DrawRulesScoring() const
{
    // Show the heading first.
    DrawText("SCORING", 70, 570, 30, RAYWHITE);

    // List the point values and starting lives.
    DrawText("Pellet: 10 points", 70, 615, 24, RAYWHITE);
    DrawText("Cherry: 50 points", 70, 655, 24, RAYWHITE);
    DrawText("Ghost: 200, 400, 800, 1600 points", 70, 695, 24, RAYWHITE);
    DrawText("You begin each run with 3 lives.", 70, 735, 24, RAYWHITE);
}

// Draws the full rules screen.
void Game::DrawRulesScreen() const
{
    // Clear the screen before drawing the text.
    ClearBackground(BLACK);

    // Draw the title at the top of the page.
    const char* title = "GAME RULES";
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, 56)) / 2, 50, 56, YELLOW);

    // Draw each rules section in order.
    DrawRulesControls();
    DrawRulesObjective();
    DrawRulesScoring();

    // Show how to return to the main menu.
    const char* returnHint = "PRESS ENTER, SPACE, OR ESC TO RETURN";
    DrawText(returnHint, (SCREEN_WIDTH - MeasureText(returnHint, 24)) / 2, 840, 24, YELLOW);
}

// Draws a win or game over overlay on top of the maze.
// title is the big heading, subtitle is the small line, and color is the title color.
void Game::DrawEndScreen(const char* title, const char* subtitle, Color color) const
{
    // Draw the frozen gameplay scene first.
    manager.RenderGame();

    // Draw the large end title and the smaller subtitle.
    DrawText(title, (SCREEN_WIDTH - MeasureText(title, 60)) / 2, 420, 60, color);
    DrawText(subtitle, (SCREEN_WIDTH - MeasureText(subtitle, 26)) / 2, 500, 26, RAYWHITE);
}

// Updates the main menu state.
void Game::UpdateMainMenu()
{
    // Count time so the blinking hint can animate.
    stateTimer = stateTimer + deltaTime;

    // Move the selection up when the user presses the up arrow.
    if (IsKeyPressed(KEY_UP))
    {
        menuSelection = menuSelection - 1;
        if (menuSelection < 0)
        {
            menuSelection = MENU_OPTION_COUNT - 1;
        }
        return;
    }

    // Move the selection down when the user presses the down arrow.
    if (IsKeyPressed(KEY_DOWN))
    {
        menuSelection = menuSelection + 1;
        if (menuSelection >= MENU_OPTION_COUNT)
        {
            menuSelection = 0;
        }
        return;
    }

    // Confirm the current option when Enter or Space is pressed.
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        if (menuSelection == 0)
        {
            manager.ResetLevel();
            SetState(PlayingState);
        }
        else if (menuSelection == 1)
        {
            SetState(RulesState);
        }
        else
        {
            CloseWindow();
        }
        return;
    }

    // Allow Escape to close the program from the main menu.
    if (IsKeyPressed(KEY_ESCAPE))
    {
        CloseWindow();
    }
}

// Updates the rules screen state.
void Game::UpdateRulesState()
{
    // Count time here too so the screen can keep simple timing behavior.
    stateTimer = stateTimer + deltaTime;

    // Return to the menu when the user presses one of the return keys.
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        SetState(MainMenuState);
    }
}

// Updates the active gameplay state.
void Game::UpdatePlayingState()
{
    // Let the gameplay manager update Pac-Man, ghosts, pellets, and HUD values.
    manager.UpdateGame();
}

// Updates the game over and victory states.
void Game::UpdateEndingState()
{
    // Count time so the ending screen can auto-return to the menu.
    stateTimer = stateTimer + deltaTime;

    // Return immediately if the timer is done or the player confirms early.
    if (stateTimer >= END_SCREEN_TIME || IsKeyPressed(KEY_ENTER) ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE))
    {
        SetState(MainMenuState);
    }
}

// Creates the window and runs the main game loop.
void Game::start()
{
    // Open the window and fix the game to 60 frames per second.
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Pac-Man");
    SetTargetFPS(60);

    // Load textures before the loop begins.
    LoadAssets();

    // Start on the main menu.
    state = MainMenuState;
    stateTimer = 0.0f;

    // Run until the player closes the window.
    while (!WindowShouldClose())
    {
        // Store the frame time, then update and draw one frame.
        deltaTime = GetFrameTime();
        update();
        BeginDrawing();
        render();
        EndDrawing();
    }

    // Clean up after the loop exits.
    end();
}

// Updates the game based on the current screen state.
void Game::update()
{
    // Send control to the matching state update function.
    if (state == MainMenuState)
    {
        UpdateMainMenu();
        return;
    }

    if (state == RulesState)
    {
        UpdateRulesState();
        return;
    }

    if (state == PlayingState)
    {
        UpdatePlayingState();
        return;
    }

    // Any other state is an ending state.
    UpdateEndingState();
}

// Draws the correct screen for the current state.
void Game::render()
{
    // Draw the screen that belongs to the current state.
    if (state == MainMenuState)
    {
        DrawMenu();
    }
    else if (state == RulesState)
    {
        DrawRulesScreen();
    }
    else if (state == PlayingState)
    {
        manager.RenderGame();
    }
    else if (state == GameOverState)
    {
        DrawEndScreen("GAME OVER", "Returning to menu...", RED);
    }
    else if (state == VictoryState)
    {
        DrawEndScreen("YOU WIN!", "Returning to menu...", GREEN);
    }
}

// Frees assets and closes the window.
void Game::end()
{
    // Unload textures before closing the window.
    UnloadAssets();

    // Close the window only if raylib still reports it as open.
    if (IsWindowReady())
    {
        CloseWindow();
    }
}

// Returns the frame time from the last update.
float Game::GetDeltaTime() const
{
    // Give the gameplay code the stored delta time.
    return deltaTime;
}

// Returns the current screen state.
Game::StateType Game::GetState() const
{
    // Give back the stored game state.
    return state;
}

// Changes the screen state and resets the shared state timer.
// nextState is the new state that should become active.
void Game::SetState(StateType nextState)
{
    // Store the new state and restart the timer.
    state = nextState;
    stateTimer = 0.0f;

    // Reset the menu cursor when coming back to the main menu.
    if (nextState == MainMenuState)
    {
        menuSelection = 0;
    }
}
