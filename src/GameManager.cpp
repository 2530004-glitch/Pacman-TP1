#include "GameManager.h"

#include <cmath>
#include <cstdlib>
#include <fstream>

#include "Game.h"
#include "Ghost.h"
#include "PacMan.h"
#include "Pellet.h"

static const int   GHOST_HOUSE_COL = 13;
static const int   GHOST_HOUSE_ROW = 14;
static const float LANE_SNAP_TOLERANCE = 4.0f;
static const float TURN_CENTER_TOLERANCE = 0.5f;
static const float GHOST_PROBE_DISTANCE = 2.0f;
static const float FRIGHTENED_DURATION = 10.0f;
static const float COLLISION_RADIUS = GameManager::TileSize * 0.28f;

// Crash the game with an error message. Used when a critical file is missing or corrupt.
static void FatalError(const char* message)
{
    TraceLog(LOG_ERROR, "%s", message);
    if (IsWindowReady())
        CloseWindow();
    std::exit(1);
}

// Returns true if the tile is inside the ghost house interior (not the door
static bool IsInsideGhostHouse(int column, int row)
{
    return row >= 11 && row <= 17 && column >= 9 && column <= 18;
}

// Wraps a column index so it stays within the map (for tunnels)
static int WrapColumn(int column, int width)
{
    if (column < 0)     return width - 1;
    if (column >= width) return 0;
    return column;
}

// Maps an arrow key to a movement direction.
static Vector2D KeyToDirection(int key)
{
    if (key == KEY_UP)    return MakeVector2D(0.0f, -1.0f);
    if (key == KEY_DOWN)  return MakeVector2D(0.0f,  1.0f);
    if (key == KEY_LEFT)  return MakeVector2D(-1.0f, 0.0f);
    if (key == KEY_RIGHT) return MakeVector2D(1.0f,  0.0f);
    return MakeVector2D(0.0f, 0.0f);
}

static Vector2 ToVector2(Vector2D v)
{
    return Vector2{v.x, v.y};
}

static Vector2D ToVector2D(Vector2 v)
{
    return MakeVector2D(v.x, v.y);
}

// Eating more ghosts in one frightened period multiplies the score.
static int GhostComboMultiplier(int combo)
{
    if (combo == 2) return 2;
    if (combo == 3) return 4;
    if (combo >= 4) return 8;
    return 1;
}

static std::string FindMapPath()
{
    if (FileExists("assets/map.txt"))       return "assets/map.txt";
    if (FileExists("..\\assets\\map.txt"))  return "..\\assets\\map.txt";
    return "assets/map.txt";
}

// ---------------------- Constructor / Destructor ----------------------

GameManager::GameManager(Game& owner) : game(owner)
{
    pacMan           = NULL;
    pacManTexture    = NULL;
    coinTexture      = NULL;
    bigCoinTexture   = NULL;
    inkyTexture      = NULL;
    pinkyTexture     = NULL;
    clydeTexture     = NULL;
    frightenedTexture = NULL;
    wallTexture      = NULL;
    frightenedTimer  = 0.0f;
    currentLives     = 3;
}

GameManager::~GameManager()
{
    ClearObjects();
}

// ---------------------- Map loading ----------------------

void GameManager::LoadMap()
{
    std::string path = FindMapPath();
    std::ifstream file(path.c_str());
    if (!file.is_open())
        FatalError(("Failed to open map file: " + path).c_str());

    mapRows.clear();
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        mapRows.push_back(line);
    }

    if ((int)mapRows.size() != 31)
        FatalError("Map must be exactly 31 rows.");
    for (int row = 0; row < (int)mapRows.size(); row++)
        if ((int)mapRows[row].size() != 28)
            FatalError("Map must be exactly 28 columns wide.");
}

// ---------------------- Object management ----------------------

void GameManager::ClearObjects()
{
    for (int i = 0; i < (int)gameObjects.size(); i++)
        delete gameObjects[i];
    gameObjects.clear();
    ghosts.clear();

    if (pacMan != NULL)
    {
        delete pacMan;
        pacMan = NULL;
    }
}

void GameManager::AddObject(GameObject* object)
{
    gameObjects.push_back(object);
}

void GameManager::RemoveObject(GameObject* object)
{
    if (object == NULL)
        return;

    // Also remove from the ghost list if it is a ghost.
    if (object->GetTag() == TAG_GHOST)
    {
        for (int i = 0; i < (int)ghosts.size(); i++)
        {
            if (ghosts[i] == (Ghost*)object)
            {
                ghosts.erase(ghosts.begin() + i);
                break;
            }
        }
    }

    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        if (gameObjects[i] == object)
        {
            delete gameObjects[i];
            gameObjects.erase(gameObjects.begin() + i);
            return;
        }
    }
}

// ---------------------- Level setup ----------------------

// Reads the map grid and places a pellet on every '.' and '&' tile
void GameManager::SpawnPelletsFromMap()
{
    for (int row = 0; row < GetMapHeight(); row++)
    {
        for (int col = 0; col < GetMapWidth(); col++)
        {
            char cell = mapRows[row][col];
            if (cell == '.')
            {
                Vector2D pos = TileToWorldCenter(col, row);
                AddObject(new Pellet(this, pos, false, coinTexture));
            }
            else if (cell == '&')
            {
                Vector2D pos = TileToWorldCenter(col, row);
                AddObject(new Pellet(this, pos, true, bigCoinTexture));
            }
        }
    }
}

// Creates Pac-Man and all three ghosts at their starting tiles.
void GameManager::SpawnActors()
{
    // Pac-Man starts near the bottom-center of the maze.
    Vector2D pacStart = TileToWorldCenter(13, 23);
    if (pacMan != NULL)
        delete pacMan;
    pacMan = new PacMan(ToVector2(pacStart), *pacManTexture, currentLives);

    // Each ghost has a type, a spawn tile, a patrol corner tile, and its own texture.
    struct GhostSpawn { GhostType type; int sc, sr, tc, tr; Texture2D** tex; };
    GhostSpawn spawns[] = {
        { GHOST_INKY,  11, 14, 24,  5, &inkyTexture  },
        { GHOST_PINKY, 13, 14,  3,  5, &pinkyTexture  },
        { GHOST_CLYDE, 15, 14,  3, 27, &clydeTexture  }
    };

    for (int i = 0; i < 3; i++)
    {
        GhostSpawn& s = spawns[i];
        Vector2D spawn  = TileToWorldCenter(s.sc, s.sr);
        Vector2D patrol = TileToWorldCenter(s.tc, s.tr);
        Ghost* ghost = new Ghost(this, s.type, spawn, patrol, *s.tex, frightenedTexture);
        AddObject(ghost);
        ghosts.push_back(ghost);
    }
}

void GameManager::ResetLevel()
{
    ClearObjects();
    frightenedTimer = 0.0f;
    currentLives = 3;
    LoadMap();
    SpawnPelletsFromMap();
    SpawnActors();
}

void GameManager::ResetActors()
{
    frightenedTimer = 0.0f;

    if (pacMan != NULL)
    {
        delete pacMan;
        pacMan = NULL;
    }

    Vector2D pacStart = TileToWorldCenter(13, 23);
    pacMan = new PacMan(ToVector2(pacStart), *pacManTexture, currentLives);

    for (int i = 0; i < (int)ghosts.size(); i++)
        ghosts[i]->ResetPosition();
}

// ---------------------- Input ----------------------

void GameManager::HandleInput()
{
    // Input for Pac-Man is handled by PacMan::Update(); no extra pathfinding required here.
}

// ---------------------- Collisions ----------------------

void GameManager::HandlePelletCollisions()
{
    if (pacMan == NULL)
        return;

    Vector2D pacPos = ToVector2D(pacMan->GetPosition());
    for (int i = 0; i < (int)gameObjects.size(); i++)
    {
        GameObject* obj = gameObjects[i];
        if (!obj->IsActive() || obj->GetTag() != TAG_PELLET)
            continue;

        Pellet* pellet = (Pellet*)obj;
        if (!IsSameTile(pacPos, pellet->GetPosition()))
            continue;

        pellet->SetActive(false);
        if (pellet->IsSuper())
        {
            pacMan->AddScore(50);
            ActivateFrightenedMode();
        }
        else
        {
            pacMan->AddScore(10);
        }
    }
}

void GameManager::HandleGhostCollisions()
{
    if (pacMan == NULL)
        return;

    Vector2D pacPos = ToVector2D(pacMan->GetPosition());
    for (int i = 0; i < (int)ghosts.size(); i++)
    {
        Ghost* ghost = ghosts[i];
        if (!ghost->IsActive() || !IsSameTile(pacPos, ghost->GetPosition()))
            continue;

        if (ghost->IsFrightened())
        {
            ghost->SetEaten();
            pacMan->AddScore(200);
            continue;
        }

        if (!ghost->IsEaten())
        {
            currentLives--;
            if (currentLives <= 0)
                game.SetState(Game::GameOver);
            else
                ResetActors();
            return;
        }
    }
}

// ---------------------- Frightened mode ----------------------

void GameManager::ActivateFrightenedMode()
{
    frightenedTimer = FRIGHTENED_DURATION;
    for (int i = 0; i < (int)ghosts.size(); i++)
        ghosts[i]->SetFrightened();
}

void GameManager::UpdateFrightenedMode()
{
    if (frightenedTimer <= 0.0f)
        return;

    frightenedTimer -= GetDeltaTime();
    if (frightenedTimer <= 0.0f)
    {
        frightenedTimer = 0.0f;
        for (int i = 0; i < (int)ghosts.size(); i++)
            if (ghosts[i]->IsFrightened())
                ghosts[i]->SetChase();
    }
}

// ---------------------- Update / Render ----------------------

void GameManager::UpdateGame()
{
    HandleInput();
    UpdateFrightenedMode();

    if (pacMan != NULL)
    {
        Vector2 oldPos = pacMan->GetPosition();
        pacMan->Update();
        Vector2 newPos = pacMan->GetPosition();
        Vector2D tile = WorldToTile(ToVector2D(newPos));
        if (!IsWalkableTile((int)tile.x, (int)tile.y, false, false))
            pacMan->SetPosition(oldPos);
    }

    for (int i = 0; i < (int)gameObjects.size(); i++)
        if (gameObjects[i]->IsActive())
            gameObjects[i]->Update();

    HandlePelletCollisions();
    HandleGhostCollisions();

    if (CountRemainingPellets() == 0 && game.GetState() == Game::Playing)
        game.SetState(Game::Victory);
}

void GameManager::RenderGame() const
{
    ClearBackground(BLACK);
    DrawMaze();

    if (pacMan != NULL)
        pacMan->Draw();

    for (int i = 0; i < (int)gameObjects.size(); i++)
        if (gameObjects[i]->IsActive())
            gameObjects[i]->Render();

    DrawHud();
}

void GameManager::DrawMaze() const
{
    Rectangle source = { 0, 0, 16, 16 };
    for (int row = 0; row < GetMapHeight(); row++)
    {
        for (int col = 0; col < GetMapWidth(); col++)
        {
            if (mapRows[row][col] != '#')
                continue;

            float x = (float)(col * TileSize) + TileSize * 0.5f;
            float y = (float)(HudHeight + row * TileSize) + TileSize * 0.5f;
            Rectangle dest = { x, y, (float)TileSize, (float)TileSize };
            Vector2 origin = { TileSize * 0.5f, TileSize * 0.5f };
            DrawTexturePro(*wallTexture, source, dest, origin, 0.0f, WHITE);
        }
    }
}

void GameManager::DrawHud() const
{
    if (pacMan == NULL)
        return;

    DrawText(TextFormat("SCORE: %d", pacMan->GetScore()), 20,  18, 28, RAYWHITE);
    DrawText(TextFormat("LIVES: %d", currentLives),  260, 18, 28, RAYWHITE);
}

// ---------------------- Texture setup ----------------------

void GameManager::SetTextures(
    Texture2D* pacTexture,
    Texture2D* normalPelletTexture,
    Texture2D* superPelletTexture,
    Texture2D* greenGhostTexture,
    Texture2D* yellowGhostTexture,
    Texture2D* orangeGhostTexture,
    Texture2D* blueGhostTexture,
    Texture2D* mazeTexture)
{
    pacManTexture     = pacTexture;
    coinTexture       = normalPelletTexture;
    bigCoinTexture    = superPelletTexture;
    inkyTexture       = greenGhostTexture;
    pinkyTexture      = yellowGhostTexture;
    clydeTexture      = orangeGhostTexture;
    frightenedTexture = blueGhostTexture;
    wallTexture       = mazeTexture;
}

// ---------------------- Getters ----------------------

float GameManager::GetDeltaTime() const { return game.GetDeltaTime(); }

int GameManager::GetMapWidth() const
{
    return mapRows.empty() ? 28 : (int)mapRows[0].size();
}

int GameManager::GetMapHeight() const
{
    return mapRows.empty() ? 31 : (int)mapRows.size();
}

PacMan* GameManager::GetPacMan() const { return pacMan; }

Vector2D GameManager::GetGhostHouseTarget() const
{
    return TileToWorldCenter(GHOST_HOUSE_COL, GHOST_HOUSE_ROW);
}

bool GameManager::IsTileInsideInkyQuadrant(int column, int row) const
{
    return column >= GetMapWidth() / 2 && row < GetMapHeight() / 2;
}

// ---------------------- Coordinate helpers ----------------------

Vector2D GameManager::WorldToTile(Vector2D position) const
{
    float col = std::floor(position.x / (float)TileSize);
    float row = std::floor((position.y - (float)HudHeight) / (float)TileSize);
    return MakeVector2D(col, row);
}

Vector2D GameManager::TileToWorldCenter(int column, int row) const
{
    float x = (float)(column * TileSize) + TileSize * 0.5f;
    float y = (float)(HudHeight + row * TileSize) + TileSize * 0.5f;
    return MakeVector2D(x, y);
}

// Returns the tile center where the moving object will be able to turn next
Vector2D GameManager::GetNextTurnCenter(Vector2D worldPos, Vector2D dir) const
{
    Vector2D tile = WorldToTile(worldPos);
    int col = (int)tile.x;
    int row = (int)tile.y;
    Vector2D center = TileToWorldCenter(col, row);

    if (dir.x > 0.0f && worldPos.x > center.x + TURN_CENTER_TOLERANCE) col++;
    else if (dir.x < 0.0f && worldPos.x < center.x - TURN_CENTER_TOLERANCE) col--;
    else if (dir.y > 0.0f && worldPos.y > center.y + TURN_CENTER_TOLERANCE) row++;
    else if (dir.y < 0.0f && worldPos.y < center.y - TURN_CENTER_TOLERANCE) row--;

    col = WrapColumn(col, GetMapWidth());
    return TileToWorldCenter(col, row);
}

bool GameManager::CanEnterNeighborTile(Vector2D worldPos, Vector2D dir, bool useDoor, bool useHouse) const
{
    if (IsZeroVector(dir))
        return false;

    Vector2D tile = WorldToTile(worldPos);
    int col = WrapColumn((int)tile.x + (int)dir.x, GetMapWidth());
    int row = (int)tile.y + (int)dir.y;
    return IsWalkableTile(col, row, useDoor, useHouse);
}

// Snaps the position to the center of its lane perpendicular to the movement direction
void GameManager::AlignToTileCenter(Vector2D& worldPos, Vector2D dir) const
{
    Vector2D tile = WorldToTile(worldPos);
    Vector2D center = TileToWorldCenter((int)tile.x, (int)tile.y);

    bool nearX = std::fabs(center.x - worldPos.x) <= LANE_SNAP_TOLERANCE;
    bool nearY = std::fabs(center.y - worldPos.y) <= LANE_SNAP_TOLERANCE;

    if (dir.x != 0.0f && nearY) worldPos.y = center.y;
    else if (dir.y != 0.0f && nearX) worldPos.x = center.x;
    else if (IsZeroVector(dir))
    {
        if (nearX) worldPos.x = center.x;
        if (nearY) worldPos.y = center.y;
    }
}

bool GameManager::IsNearTileCenter(Vector2D worldPos, float tolerance) const
{
    Vector2D tile   = WorldToTile(worldPos);
    Vector2D center = TileToWorldCenter((int)tile.x, (int)tile.y);
    return std::fabs(center.x - worldPos.x) <= tolerance &&
           std::fabs(center.y - worldPos.y) <= tolerance;
}

// Wraps a world position through the left/right tunnel exits.
void GameManager::WrapPosition(Vector2D& worldPos) const
{
    float leftExit   = -TileSize * 0.5f;
    float rightExit  = (float)(GetMapWidth() * TileSize) - TileSize * 0.5f;
    float leftCenter = TileSize * 0.5f;
    float rightCenter = (float)(GetMapWidth() * TileSize) - TileSize * 0.5f;

    if (worldPos.x < leftExit) worldPos.x = rightCenter;
    else if (worldPos.x > rightExit) worldPos.x = leftCenter;
}

// ---------------------- Movement ----------------------

bool GameManager::IsWalkableTile(int column, int row, bool useDoor, bool useHouse) const
{
    if (row < 0 || row >= GetMapHeight()) return false;
    if (column < 0 || column >= GetMapWidth()) return false;

    char cell = mapRows[row][column];
    if (cell == '#') return false;
    if (cell == '-') return useDoor;
    if (cell == ' ' && IsInsideGhostHouse(column, row)) return useHouse;
    return true;
}

bool GameManager::IsSameTile(Vector2D a, Vector2D b) const
{
    return AreVectorsEqual(WorldToTile(a), WorldToTile(b));
}

// Returns true if moving from pos in dir by distance stays on walkable tiles.
bool GameManager::CanMove(Vector2D pos, Vector2D dir, float distance, bool useDoor, bool useHouse) const
{
    if (IsZeroVector(dir))
        return false;

    // Move the position and check all tiles touched by the collision radius
    Vector2D moved = pos;
    moved.x += dir.x * distance;
    moved.y += dir.y * distance;
    WrapPosition(moved);

    float r    = COLLISION_RADIUS;
    int left   = (int)std::floor((moved.x - r) / TileSize);
    int right  = (int)std::floor((moved.x + r) / TileSize);
    int top    = (int)std::floor((moved.y - HudHeight - r) / TileSize);
    int bottom = (int)std::floor((moved.y - HudHeight + r) / TileSize);

    for (int row = top; row <= bottom; row++)
        for (int col = left; col <= right; col++)
            if (!IsWalkableTile(WrapColumn(col, GetMapWidth()), row, useDoor, useHouse))
                return false;

    return true;
}

int GameManager::CountRemainingPellets() const
{
    int count = 0;
    for (int i = 0; i < (int)gameObjects.size(); i++)
        if (gameObjects[i]->IsActive() && gameObjects[i]->GetTag() == TAG_PELLET)
            count++;
    return count;
}

// ---------------------- Ghost direction logic ----------------------

// Picks the next direction for a ghost. In frightened mode the choice is random;
// otherwise it picks whichever valid direction gets closest to the target tile
Vector2D GameManager::ChooseGhostDirection(
    const Ghost* ghost,
    Vector2D target,
    bool randomize,
    bool allowReverse) const
{
    Vector2D dirs[4] = {
        MakeVector2D(0.0f, -1.0f),
        MakeVector2D(-1.0f, 0.0f),
        MakeVector2D(0.0f,  1.0f),
        MakeVector2D(1.0f,  0.0f)
    };
    Vector2D reverse = MakeVector2D(-ghost->GetDirection().x, -ghost->GetDirection().y);
    bool canUseDoor  = ghost->IsEaten();

    // Show every direction the ghost is allowed to move in
    std::vector<Vector2D> valid;
    for (int i = 0; i < 4; i++)
    {
        if (!allowReverse && AreVectorsEqual(dirs[i], reverse))
            continue;
        if (CanEnterNeighborTile(ghost->GetPosition(), dirs[i], canUseDoor, canUseDoor) &&
            CanMove(ghost->GetPosition(), dirs[i], GHOST_PROBE_DISTANCE, canUseDoor, canUseDoor))
        {
            valid.push_back(dirs[i]);
        }
    }

    // If nothing is valid, allow the reverse as a last resort
    if (valid.empty())
        valid.push_back(reverse);

    if (randomize)
        return valid[GetRandomValue(0, (int)valid.size() - 1)];

    // Pick whichever direction puts the ghost closest to its target.
    Vector2D targetTile = WorldToTile(target);
    Vector2D ghostTile = WorldToTile(ghost->GetPosition());
    Vector2D best = valid[0];
    int bestDist = 999999;

    for (int i = 0; i < (int)valid.size(); i++)
    {
        Vector2D next = AddVectors(ghostTile, valid[i]);
        int dist = (int)(std::fabs(next.x - targetTile.x) + std::fabs(next.y - targetTile.y));
        if (dist < bestDist)
        {
            bestDist = dist;
            best     = valid[i];
        }
    }

    return best;
}
