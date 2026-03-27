#include "Game.h"

#include <cmath>
#include <cstdlib>
#include <string>

static const int   SCREEN_WIDTH      = 28 * GameManager::TileSize;
static const int   SCREEN_HEIGHT     = 31 * GameManager::TileSize + GameManager::HudHeight;
static const float END_SCREEN_TIME   = 3.0f;

// ---------------------- Static helpers ----------------------

static void ClearTexture(Texture2D* t)
{
    t->id = 0; t->width = 0; t->height = 0; t->mipmaps = 0; t->format = 0;
}

// Tries to find an asset in the project root, then one folder up (for Deployment runs).
static std::string ResolveAsset(const char* path)
{
    if (FileExists(path))
        return path;
    std::string up = std::string("..\\") + path;
    if (FileExists(up.c_str()))
        return up;
    return path;
}

// Draws a filled circle into a raylib Image. Used to build the cherry sprite.
static void FillCircle(Image* img, int cx, int cy, int r, Color color)
{
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++)
            if ((x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r)
                ImageDrawPixel(img, x, y, color);
}

// ---------------------- Asset loading ----------------------

void Game::LoadAssets()
{
    if (assetsLoaded)
        return;

    // Load each texture. If a required file is missing the game exits with an error.
    auto loadRequired = [](Texture2D* t, const char* path)
    {
        std::string resolved = ResolveAsset(path);
        *t = LoadTexture(resolved.c_str());
        if (t->id == 0)
        {
            TraceLog(LOG_ERROR, "Missing texture: %s", resolved.c_str());
            if (IsWindowReady()) CloseWindow();
            std::exit(1);
        }
    };

    loadRequired(&pacManTexture,     "assets/PacMan.png");
    loadRequired(&coinTexture,       "assets/Coin.png");
    loadRequired(&inkyTexture,       "assets/greenGhost.png");
    loadRequired(&pinkyTexture,      "assets/yellowGhost.png");
    loadRequired(&clydeTexture,      "assets/orangeGhost.png");
    loadRequired(&frightenedTexture, "assets/blueGhost.png");
    loadRequired(&wallTexture,       "assets/Tileset.png");

    // The cherry (super pellet) texture is optional — build one in memory if the file is missing.
    std::string cherryPath = ResolveAsset("assets/Cherry.png");
    if (FileExists(cherryPath.c_str()))
    {
        bigCoinTexture = LoadTexture(cherryPath.c_str());
    }
    else
    {
        Image img = GenImageColor(16, 16, BLANK);
        FillCircle(&img, 5, 10, 4, RED);
        FillCircle(&img, 10, 10, 4, RED);
        for (int y = 2; y <= 6; y++) ImageDrawPixel(&img, 7, y, DARKGREEN);
        for (int i = 0; i <= 3; i++)
        {
            ImageDrawPixel(&img, 7 - i, 5 - i, DARKGREEN);
            ImageDrawPixel(&img, 7 + i, 5 - i, DARKGREEN);
        }
        for (int x = 6; x <= 8; x++) ImageDrawPixel(&img, x, 2, GREEN);
        bigCoinTexture = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    manager.SetTextures(&pacManTexture, &coinTexture, &bigCoinTexture, &inkyTexture,
                        &pinkyTexture, &clydeTexture, &frightenedTexture, &wallTexture);
    assetsLoaded = true;
}

void Game::UnloadAssets()
{
    if (!assetsLoaded)
        return;

    Texture2D* textures[] = {
        &pacManTexture, &coinTexture, &bigCoinTexture, &inkyTexture,
        &pinkyTexture, &clydeTexture, &frightenedTexture, &wallTexture
    };
    for (int i = 0; i < 8; i++)
    {
        if (textures[i]->id > 0)
            UnloadTexture(*textures[i]);
        ClearTexture(textures[i]);
    }
    assetsLoaded = false;
}

// ---------------------- Constructor / Destructor ----------------------

Game::Game() : manager(*this)
{
    ClearTexture(&pacManTexture);
    ClearTexture(&coinTexture);
    ClearTexture(&bigCoinTexture);
    ClearTexture(&inkyTexture);
    ClearTexture(&pinkyTexture);
    ClearTexture(&clydeTexture);
    ClearTexture(&frightenedTexture);
    ClearTexture(&wallTexture);

    deltaTime      = 0.0f;
    stateTimer     = 0.0f;
    state          = MainMenu;
    menuSelection  = 0;
    assetsLoaded   = false;
}

Game::~Game()
{
    end();
}

// ---------------------- Drawing ----------------------

// Draws a single menu button. Selected buttons are highlighted
static void DrawMenuButton(int cx, int y, int w, int h, const char* label, bool selected)
{
    Color bg     = selected ? Color{55, 85, 190, 255} : Color{25, 25, 45, 255};
    Color border = selected ? Color{110, 145, 255, 255} : Color{50, 50, 80, 255};

    DrawRectangle(cx - w / 2, y, w, h, bg);
    DrawRectangleLinesEx({ (float)(cx - w / 2), (float)y, (float)w, (float)h }, 2.0f, border);

    // Small arrow indicator on the left side of the selected button
    if (selected)
        DrawText(">", cx - w / 2 + 14, y + h / 2 - 10, 20, Color{150, 175, 255, 255});

    int fontSize = 24;
    DrawText(label, cx - MeasureText(label, fontSize) / 2, y + h / 2 - fontSize / 2, fontSize, RAYWHITE);
}

void Game::DrawMenu() const
{
    int cx = SCREEN_WIDTH / 2;

    ClearBackground(Color{10, 10, 20, 255});
    DrawRectangle(cx - 210, 70, 420, 530, Color{18, 18, 35, 255});
    DrawRectangleLinesEx({ (float)(cx - 210), 70, 420, 530 }, 1.5f, Color{40, 40, 65, 255});

    // Title
    const char* title = "PAC-MAN";
    int titleSize = 64;
    DrawText(title, cx - MeasureText(title, titleSize) / 2, 110, titleSize, RAYWHITE);
    DrawRectangle(cx - 50, 185, 100, 2, Color{80, 115, 240, 255});

    // Tagline
    const char* tag = "EAT ALL THE PELLETS, AVOID THE GHOSTS!";
    DrawText(tag, cx - MeasureText(tag, 18) / 2, 200, 18, Color{120, 120, 155, 255});

    // The three menu buttons
    const char* labels[] = { "PLAY", "HOW TO PLAY", "QUIT" };
    int buttonY[] = { 268, 348, 428 };
    for (int i = 0; i < 3; i++)
        DrawMenuButton(cx, buttonY[i], 300, 58, labels[i], menuSelection == i);

    const char* hint = "Arrow Keys  Navigate     Enter  Confirm";
    DrawText(hint, cx - MeasureText(hint, 17) / 2, 520, 17, Color{80, 80, 110, 255});
}

void Game::DrawRulesScreen() const
{
    int cx = SCREEN_WIDTH / 2;
    ClearBackground(Color{10, 10, 20, 255});

    // Page title
    const char* title = "HOW TO PLAY";
    DrawText(title, cx - MeasureText(title, 52) / 2, 55, 52, RAYWHITE);
    DrawRectangle(cx - 60, 118, 120, 2, Color{80, 115, 240, 255});

    // Controls section
    DrawText("CONTROLS", 70, 188, 26, Color{110, 145, 255, 255});
    DrawText("Use the arrow keys to move Pac-Man.", 70, 216, 21, Color{200, 200, 215, 255});

    // Objective section
    DrawText("OBJECTIVE", 70, 270, 26, Color{110, 145, 255, 255});
    DrawText("Eat every pellet in the maze to win.", 70, 308, 21, Color{200, 200, 215, 255});
    DrawText("Touching a ghost will kill you.", 70, 336, 21, Color{200, 200, 215, 255});
    DrawText("Cherries turn ghosts blue for a short time.", 70, 364, 21, Color{200, 200, 215, 255});
    DrawText("Blue ghosts can be eaten for bonus points.", 70, 392, 21, Color{200, 200, 215, 255});

    // Scoring section
    DrawText("SCORING", 70, 446, 26, Color{110, 145, 255, 255});
    DrawText("Regular pellet   10 pts", 70, 484, 21, Color{200, 200, 215, 255});
    DrawText("Cherry     50 pts", 70, 512, 21, Color{200, 200, 215, 255});
    DrawText("Ghost            200 / 400 / 800 / 1600 pts (combo)", 70, 540, 21, Color{200, 200, 215, 255});
    DrawText("You have 3 lives.", 70, 568, 21, Color{200, 200, 215, 255});

    // Return hint.
    const char* back = "Press Enter, Space, or Esc to go back";
    DrawText(back, cx - MeasureText(back, 19) / 2, 650, 19, Color{80, 80, 110, 255});
}

// Overlays a win or game-over message on top of the frozen gameplay scene.
void Game::DrawEndScreen(const char* title, const char* subtitle, Color color) const
{
    manager.RenderGame();

    int cx = SCREEN_WIDTH / 2;

    // Semi-transparent backdrop so the text is readable over the maze.
    DrawRectangle(cx - 240, 390, 480, 140, Color{0, 0, 0, 180});

    DrawText(title,    cx - MeasureText(title,    56) / 2, 405, 56, color);
    DrawText(subtitle, cx - MeasureText(subtitle, 22) / 2, 478, 22, RAYWHITE);
}

// ---------------------- Update logic ----------------------

void Game::UpdateMenu()
{
    stateTimer += deltaTime;

    if (IsKeyPressed(KEY_UP))
    {
        menuSelection = (menuSelection - 1 + 3) % 3;
        return;
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        menuSelection = (menuSelection + 1) % 3;
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))
    {
        if      (menuSelection == 0) { manager.ResetLevel(); SetState(Playing); }
        else if (menuSelection == 1) { SetState(Rules); }
        else                         { CloseWindow(); }
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE))
        CloseWindow();
}

void Game::UpdateRules()
{
    stateTimer += deltaTime;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE) ||
        IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE))
    {
        SetState(MainMenu);
    }
}

void Game::UpdatePlaying()
{
    manager.UpdateGame();
}

void Game::UpdateEnding()
{
    stateTimer += deltaTime;
    if (stateTimer >= END_SCREEN_TIME ||
        IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE))
    {
        SetState(MainMenu);
    }
}

// ---------------------- Main loop ----------------------

void Game::start()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "PacMan-TP1");
    SetTargetFPS(60);
    LoadAssets();

    state = MainMenu;
    stateTimer = 0.0f;

    while (!WindowShouldClose())
    {
        deltaTime = GetFrameTime();
        update();
        BeginDrawing();
        render();
        EndDrawing();
    }

    end();
}

void Game::update()
{
    if (state == MainMenu) {
        UpdateMenu();   
        return; 
    }
    if (state == Rules) {
        UpdateRules();  
        return; 
    }

    if (state == Playing) {
        UpdatePlaying(); 
        return; 
    }
    
    UpdateEnding();
}

void Game::render()
{
    if      (state == MainMenu) DrawMenu();
    else if (state == Rules)    DrawRulesScreen();
    else if (state == Playing)  manager.RenderGame();
    else if (state == GameOver) DrawEndScreen("GAME OVER", "Returning to menu...", RED);
    else if (state == Victory)  DrawEndScreen("YOU WIN!",  "Returning to menu...", Color{100, 220, 100, 255});
}

void Game::end()
{
    UnloadAssets();
    if (IsWindowReady())
        CloseWindow();
}

// ---------------------- Getters / Setters ----------------------

float Game::GetDeltaTime() const { return deltaTime; }
Game::State Game::GetState() const { return state; }

void Game::SetState(State next)
{
    state      = next;
    stateTimer = 0.0f;
    if (next == MainMenu)
        menuSelection = 0;
}
