/*
    GameManager.cpp
    ---------------
    This file implements the GameManager class.
    The GameManager loads the map, creates gameplay objects, handles input,
    collisions, frightened mode, ghost movement choices, and maze rendering.
*/

#include "GameManager.h"

#include <cmath>
#include <cstdlib>
#include <fstream>

#include "Game.h"
#include "GameObject.h"
#include "Ghost.h"
#include "PacMan.h"
#include "Pellet.h"

static const int DEFAULT_MAP_WIDTH = 28;
static const int DEFAULT_MAP_HEIGHT = 31;
static const int GHOST_HOUSE_TARGET_COLUMN = 13;
static const int GHOST_HOUSE_TARGET_ROW = 14;
static const float LANE_SNAP_TOLERANCE = 4.0f;
static const float TURN_CENTER_TOLERANCE = 0.5f;
static const float GHOST_DIRECTION_PROBE_DISTANCE = 2.0f;
static const float PACMAN_FRIGHTENED_DURATION = 10.0f;
static const float COLLISION_RADIUS = GameManager::TileSize * 0.28f;

// Stops the game after printing an error message.
// text is the message to send to the raylib log.
static void StopGameWithMessage(const char* text)
{
    TraceLog(LOG_ERROR, "%s", text);
    if (IsWindowReady())
    {
        CloseWindow();
    }

    std::exit(1);
}

// Returns true if a tile is part of the inside of the ghost house.
// column and row are tile coordinates in the map grid.
static bool IsGhostHouseTileInternal(int column, int row)
{
    if (row < 11 || row > 17)
    {
        return false;
    }

    if (column < 9 || column > 18)
    {
        return false;
    }

    return true;
}

// Returns the reverse of a direction.
// direction is the input direction to flip.
static Vector2D GetOppositeDirection(Vector2D direction)
{
    return MakeVector2D(-direction.x, -direction.y);
}

// Returns true if two directions point exactly opposite ways.
// firstDirection and secondDirection are the two directions to compare.
static bool AreDirectionsOpposite(Vector2D firstDirection, Vector2D secondDirection)
{
    if (firstDirection.x + secondDirection.x != 0.0f)
    {
        return false;
    }

    if (firstDirection.y + secondDirection.y != 0.0f)
    {
        return false;
    }

    return true;
}

// Converts a raylib key code into one of the four movement directions.
// key is the key code returned by raylib.
static Vector2D DirectionFromKey(int key)
{
    if (key == KEY_UP)
    {
        return MakeVector2D(0.0f, -1.0f);
    }

    if (key == KEY_DOWN)
    {
        return MakeVector2D(0.0f, 1.0f);
    }

    if (key == KEY_LEFT)
    {
        return MakeVector2D(-1.0f, 0.0f);
    }

    if (key == KEY_RIGHT)
    {
        return MakeVector2D(1.0f, 0.0f);
    }

    return MakeVector2D(0.0f, 0.0f);
}

// Wraps a map column to the left or right tunnel edge.
// column is the column to fix, and width is the map width.
static int WrapColumnIndex(int column, int width)
{
    if (column < 0)
    {
        return width - 1;
    }

    if (column >= width)
    {
        return 0;
    }

    return column;
}

// Reads the newest pressed arrow key direction, if any.
// direction points to the output direction.
static bool TryReadPressedArrowDirection(Vector2D* direction)
{
    bool foundDirection = false;

    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed())
    {
        Vector2D candidate = DirectionFromKey(key);
        if (!IsZeroVector(candidate))
        {
            *direction = candidate;
            foundDirection = true;
        }
    }

    return foundDirection;
}

// Reads one held arrow key direction, if any.
// direction points to the output direction.
static bool TryReadHeldArrowDirection(Vector2D* direction)
{
    if (IsKeyDown(KEY_UP))
    {
        *direction = MakeVector2D(0.0f, -1.0f);
        return true;
    }

    if (IsKeyDown(KEY_DOWN))
    {
        *direction = MakeVector2D(0.0f, 1.0f);
        return true;
    }

    if (IsKeyDown(KEY_LEFT))
    {
        *direction = MakeVector2D(-1.0f, 0.0f);
        return true;
    }

    if (IsKeyDown(KEY_RIGHT))
    {
        *direction = MakeVector2D(1.0f, 0.0f);
        return true;
    }

    return false;
}

// Returns the score multiplier for the ghost combo count.
// combo is how many ghosts Pac-Man has eaten in one frightened period.
static int GetGhostComboMultiplier(int combo)
{
    if (combo == 2)
    {
        return 2;
    }

    if (combo == 3)
    {
        return 4;
    }

    if (combo >= 4)
    {
        return 8;
    }

    return 1;
}

// Returns the correct map path for root and Deployment launches.
static std::string ResolveMapPath()
{
    if (FileExists("assets/map.txt"))
    {
        return "assets/map.txt";
    }

    if (FileExists("..\\assets\\map.txt"))
    {
        return "..\\assets\\map.txt";
    }

    return "assets/map.txt";
}

// Creates the gameplay manager and fills it with default values.
// owner is the Game object that owns this manager.
GameManager::GameManager(Game& owner)
    : game(owner)
{
    gameObjects.clear();
    mapRows.clear();
    ghosts.clear();
    pacMan = NULL;
    pacManTexture = NULL;
    coinTexture = NULL;
    bigCoinTexture = NULL;
    inkyTexture = NULL;
    pinkyTexture = NULL;
    clydeTexture = NULL;
    frightenedTexture = NULL;
    wallTexture = NULL;
    frightenedTimer = 0.0f;
}

// Cleans up every gameplay object owned by the manager.
GameManager::~GameManager()
{
    ClearObjects();
}

// Loads the map file from disk.
void GameManager::LoadMap()
{
    std::string mapPath = ResolveMapPath();
    std::ifstream file(mapPath.c_str());
    if (!file.is_open())
    {
        std::string message = "Failed to open map file: " + mapPath;
        StopGameWithMessage(message.c_str());
    }

    mapRows.clear();
    ReadMapRows(file);
    ValidateMapSize();
}

// Reads every line from the map file.
// file is the already opened input file.
void GameManager::ReadMapRows(std::ifstream& file)
{
    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
        {
            line.erase(line.size() - 1, 1);
        }

        mapRows.push_back(line);
    }
}

// Checks that the loaded map is the expected 28 by 31 size.
void GameManager::ValidateMapSize() const
{
    if ((int)mapRows.size() != DEFAULT_MAP_HEIGHT)
    {
        StopGameWithMessage("Map must be exactly 31 rows.");
    }

    for (int row = 0; row < (int)mapRows.size(); row++)
    {
        if ((int)mapRows[row].size() != DEFAULT_MAP_WIDTH)
        {
            StopGameWithMessage("Map must be exactly 28 columns wide.");
        }
    }
}

// Deletes every gameplay object and clears the object lists.
void GameManager::ClearObjects()
{
    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        delete gameObjects[index];
    }

    gameObjects.clear();
    ghosts.clear();
    pacMan = NULL;
}

// Creates pellets by reading the map grid.
void GameManager::SpawnPelletsFromMap()
{
    for (int row = 0; row < GetMapHeight(); row++)
    {
        SpawnPelletsInRow(row);
    }
}

// Creates pellets for one map row.
// row is the map row to inspect.
void GameManager::SpawnPelletsInRow(int row)
{
    for (int column = 0; column < GetMapWidth(); column++)
    {
        char cell = mapRows[row][column];
        CreatePelletFromCell(column, row, cell);
    }
}

// Creates a pellet if the map cell represents one.
// column and row are the tile location, and cell is the map character.
void GameManager::CreatePelletFromCell(int column, int row, char cell)
{
    if (cell == '.')
    {
        Vector2D position = TileToWorldCenter(column, row);
        Pellet* pellet = new Pellet(this, position, false, coinTexture);
        AddObject(pellet);
    }

    if (cell == '&')
    {
        Vector2D position = TileToWorldCenter(column, row);
        Pellet* pellet = new Pellet(this, position, true, bigCoinTexture);
        AddObject(pellet);
    }
}

// Creates Pac-Man and the three ghosts.
void GameManager::SpawnActors()
{
    SpawnPacMan();
    SpawnGhosts();
}

// Creates the Pac-Man object at its starting tile.
void GameManager::SpawnPacMan()
{
    Vector2D startPosition = TileToWorldCenter(13, 23);
    pacMan = new PacMan(this, startPosition, pacManTexture);
    AddObject(pacMan);
}

// Creates all three ghost objects.
void GameManager::SpawnGhosts()
{
    CreateGhost(GHOST_INKY, 11, 14, 24, 5, inkyTexture);
    CreateGhost(GHOST_PINKY, 13, 14, 3, 5, pinkyTexture);
    CreateGhost(GHOST_CLYDE, 15, 14, 3, 27, clydeTexture);
}

// Creates one ghost and adds it to the object lists.
// ghostType chooses the AI, spawnColumn/spawnRow choose the start tile,
// targetColumn/targetRow choose the patrol target, and chaseTexture is the normal sprite.
void GameManager::CreateGhost(GhostType ghostType,
                              int spawnColumn,
                              int spawnRow,
                              int targetColumn,
                              int targetRow,
                              Texture2D* chaseTexture)
{
    Vector2D startPosition = TileToWorldCenter(spawnColumn, spawnRow);
    Vector2D patrolTarget = TileToWorldCenter(targetColumn, targetRow);
    Ghost* ghost = new Ghost(this, ghostType, startPosition, patrolTarget,
                             chaseTexture, frightenedTexture);
    AddObject(ghost);
    ghosts.push_back(ghost);
}

// Reads player input and sends the chosen direction to Pac-Man.
void GameManager::HandleInput()
{
    if (pacMan == NULL)
    {
        return;
    }

    Vector2D inputDirection = MakeVector2D(0.0f, 0.0f);
    bool hasInput = TryGetPacManInput(&inputDirection);
    if (!hasInput)
    {
        return;
    }

    ApplyPacManInput(inputDirection);
}

// Reads the best current Pac-Man input direction.
// inputDirection points to the output direction.
bool GameManager::TryGetPacManInput(Vector2D* inputDirection) const
{
    bool hasPressedDirection = TryReadPressedArrowDirection(inputDirection);
    if (hasPressedDirection)
    {
        return true;
    }

    Vector2D currentDirection = pacMan->GetDirection();
    if (!IsZeroVector(currentDirection))
    {
        return false;
    }

    return TryReadHeldArrowDirection(inputDirection);
}

// Applies one input direction to Pac-Man.
// inputDirection is the direction requested by the player.
void GameManager::ApplyPacManInput(Vector2D inputDirection)
{
    Vector2D currentDirection = pacMan->GetDirection();
    if (AreDirectionsOpposite(currentDirection, inputDirection))
    {
        pacMan->QueueDirection(inputDirection);
        return;
    }

    Vector2D nextTurnCenter = GetNextTurnCenter(pacMan->GetPosition(), currentDirection);
    if (CanEnterNeighborTile(nextTurnCenter, inputDirection, false, false))
    {
        pacMan->QueueDirection(inputDirection);
    }
}

// Handles Pac-Man touching pellets.
void GameManager::HandlePelletCollisions()
{
    if (pacMan == NULL)
    {
        return;
    }

    Vector2D pacManPosition = pacMan->GetPosition();
    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        GameObject* object = gameObjects[index];
        if (!IsPelletObject(object))
        {
            continue;
        }

        Pellet* pellet = (Pellet*)object;
        if (!IsSameTile(pacManPosition, pellet->GetPosition()))
        {
            continue;
        }

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

// Handles Pac-Man touching ghosts.
void GameManager::HandleGhostCollisions()
{
    if (pacMan == NULL)
    {
        return;
    }

    Vector2D pacManPosition = pacMan->GetPosition();
    for (int index = 0; index < (int)ghosts.size(); index++)
    {
        Ghost* ghost = ghosts[index];
        if (!ghost->IsActive())
        {
            continue;
        }

        if (!IsSameTile(pacManPosition, ghost->GetPosition()))
        {
            continue;
        }

        if (ghost->IsFrightened())
        {
            ghost->SetEaten();
            pacMan->IncreaseGhostCombo();
            int combo = pacMan->GetGhostCombo();
            int points = 200 * GetGhostComboMultiplier(combo);
            pacMan->AddScore(points);
            continue;
        }

        if (!ghost->IsEaten())
        {
            pacMan->LoseLife();
            pacMan->ResetGhostCombo();
            if (pacMan->GetLives() <= 0)
            {
                game.SetState(Game::GameOverState);
            }
            else
            {
                ResetActors();
            }

            return;
        }
    }
}

// Starts frightened mode for all ghosts.
void GameManager::ActivateFrightenedMode()
{
    if (pacMan != NULL)
    {
        pacMan->ResetGhostCombo();
    }

    frightenedTimer = PACMAN_FRIGHTENED_DURATION;
    for (int index = 0; index < (int)ghosts.size(); index++)
    {
        ghosts[index]->SetFrightened();
    }
}

// Counts down frightened mode and ends it when time runs out.
void GameManager::UpdateFrightenedMode()
{
    if (frightenedTimer <= 0.0f)
    {
        return;
    }

    frightenedTimer = frightenedTimer - GetDeltaTime();
    if (frightenedTimer <= 0.0f)
    {
        EndFrightenedMode();
    }
}

// Returns every frightened ghost back to chase mode.
void GameManager::EndFrightenedMode()
{
    frightenedTimer = 0.0f;
    if (pacMan != NULL)
    {
        pacMan->ResetGhostCombo();
    }

    for (int index = 0; index < (int)ghosts.size(); index++)
    {
        Ghost* ghost = ghosts[index];
        if (ghost->IsFrightened())
        {
            ghost->SetChase();
        }
    }
}

// Updates every active gameplay object.
void GameManager::UpdateObjects()
{
    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        GameObject* object = gameObjects[index];
        if (object->IsActive())
        {
            object->Update();
        }
    }
}

// Draws every active gameplay object.
void GameManager::RenderObjects() const
{
    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        GameObject* object = gameObjects[index];
        if (object->IsActive())
        {
            object->Render();
        }
    }
}

// Draws the maze walls from the map grid.
void GameManager::DrawMaze() const
{
    for (int row = 0; row < GetMapHeight(); row++)
    {
        DrawMazeRow(row);
    }
}

// Draws all wall tiles in one map row.
// row is the row to inspect.
void GameManager::DrawMazeRow(int row) const
{
    for (int column = 0; column < GetMapWidth(); column++)
    {
        if (mapRows[row][column] == '#')
        {
            DrawWallTile(column, row);
        }
    }
}

// Draws one wall tile using the wall texture.
// column and row are the tile coordinates.
void GameManager::DrawWallTile(int column, int row) const
{
    Rectangle source;
    source.x = 0.0f;
    source.y = 0.0f;
    source.width = 16.0f;
    source.height = 16.0f;

    Rectangle destination;
    destination.x = (float)(column * TileSize) + TileSize * 0.5f;
    destination.y = (float)(HudHeight + row * TileSize) + TileSize * 0.5f;
    destination.width = (float)TileSize;
    destination.height = (float)TileSize;

    Vector2 origin;
    origin.x = TileSize * 0.5f;
    origin.y = TileSize * 0.5f;
    DrawTexturePro(*wallTexture, source, destination, origin, 0.0f, WHITE);
}

// Draws the top HUD area.
void GameManager::DrawHud() const
{
    if (pacMan == NULL)
    {
        return;
    }

    DrawScoreText();
    DrawLivesText();
}

// Draws the score text in the HUD.
void GameManager::DrawScoreText() const
{
    const char* scoreText = TextFormat("SCORE: %d", pacMan->GetScore());
    DrawText(scoreText, 20, 18, 28, RAYWHITE);
}

// Draws the lives text in the HUD.
void GameManager::DrawLivesText() const
{
    const char* livesText = TextFormat("LIVES: %d", pacMan->GetLives());
    DrawText(livesText, 260, 18, 28, RAYWHITE);
}

// Returns true if the object is an active pellet.
// object is the object to test.
bool GameManager::IsPelletObject(GameObject* object) const
{
    if (!object->IsActive())
    {
        return false;
    }

    if (object->GetTag() != TAG_PELLET)
    {
        return false;
    }

    return true;
}

// Returns true when two world positions are inside the same map tile.
// firstPosition and secondPosition are the world positions to compare.
bool GameManager::IsSameTile(Vector2D firstPosition, Vector2D secondPosition) const
{
    Vector2D firstTile = WorldToTile(firstPosition);
    Vector2D secondTile = WorldToTile(secondPosition);
    return AreVectorsEqual(firstTile, secondTile);
}

// Removes one ghost pointer from the ghost list.
// ghost is the ghost pointer to erase.
void GameManager::RemoveGhostPointer(Ghost* ghost)
{
    for (int index = 0; index < (int)ghosts.size(); index++)
    {
        if (ghosts[index] == ghost)
        {
            ghosts.erase(ghosts.begin() + index);
            return;
        }
    }
}

// Adds one object pointer to the main object list.
// object is the new object to store.
void GameManager::AddObject(GameObject* object)
{
    gameObjects.push_back(object);
}

// Removes and deletes one object.
// object is the object pointer to remove.
void GameManager::RemoveObject(GameObject* object)
{
    if (object == NULL)
    {
        return;
    }

    if (object == pacMan)
    {
        pacMan = NULL;
    }

    if (object->GetTag() == TAG_GHOST)
    {
        Ghost* ghost = (Ghost*)object;
        RemoveGhostPointer(ghost);
    }

    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        if (gameObjects[index] == object)
        {
            delete gameObjects[index];
            gameObjects.erase(gameObjects.begin() + index);
            return;
        }
    }
}

// Updates one full gameplay frame.
void GameManager::UpdateGame()
{
    HandleInput();
    UpdateFrightenedMode();
    UpdateObjects();
    HandlePelletCollisions();
    HandleGhostCollisions();

    if (CountRemainingPellets() == 0 && game.GetState() == Game::PlayingState)
    {
        game.SetState(Game::VictoryState);
    }
}

// Draws one full gameplay frame.
void GameManager::RenderGame() const
{
    ClearBackground(BLACK);
    DrawMaze();
    RenderObjects();
    DrawHud();
}

// Clears the old level and builds a fresh one.
void GameManager::ResetLevel()
{
    ClearObjects();
    frightenedTimer = 0.0f;
    LoadMap();
    SpawnPelletsFromMap();
    SpawnActors();
}

// Resets Pac-Man and all ghosts to their spawn positions.
void GameManager::ResetActors()
{
    frightenedTimer = 0.0f;
    if (pacMan != NULL)
    {
        pacMan->ResetPosition();
        pacMan->ResetGhostCombo();
    }

    for (int index = 0; index < (int)ghosts.size(); index++)
    {
        ghosts[index]->ResetPosition();
    }
}

// Stores the textures used by gameplay objects and the maze.
// Each pointer is a texture owned by the main Game object.
void GameManager::SetTextures(Texture2D* pacTexture,
                              Texture2D* normalPelletTexture,
                              Texture2D* superPelletTexture,
                              Texture2D* greenGhostTexture,
                              Texture2D* yellowGhostTexture,
                              Texture2D* orangeGhostTexture,
                              Texture2D* blueGhostTexture,
                              Texture2D* mazeTexture)
{
    pacManTexture = pacTexture;
    coinTexture = normalPelletTexture;
    bigCoinTexture = superPelletTexture;
    inkyTexture = greenGhostTexture;
    pinkyTexture = yellowGhostTexture;
    clydeTexture = orangeGhostTexture;
    frightenedTexture = blueGhostTexture;
    wallTexture = mazeTexture;
}

// Returns the frame time from the owning Game object.
float GameManager::GetDeltaTime() const
{
    return game.GetDeltaTime();
}

// Returns the tile size used by the maze.
int GameManager::GetTileSize() const
{
    return TileSize;
}

// Returns the HUD height above the maze.
int GameManager::GetHudHeight() const
{
    return HudHeight;
}

// Returns the map width in tiles.
int GameManager::GetMapWidth() const
{
    if (mapRows.empty())
    {
        return DEFAULT_MAP_WIDTH;
    }

    return (int)mapRows[0].size();
}

// Returns the map height in tiles.
int GameManager::GetMapHeight() const
{
    if (mapRows.empty())
    {
        return DEFAULT_MAP_HEIGHT;
    }

    return (int)mapRows.size();
}

// Returns the Pac-Man pointer.
PacMan* GameManager::GetPacMan() const
{
    return pacMan;
}

// Returns the ghost house home target in world coordinates.
Vector2D GameManager::GetGhostHouseTarget() const
{
    return TileToWorldCenter(GHOST_HOUSE_TARGET_COLUMN, GHOST_HOUSE_TARGET_ROW);
}

// Returns true if the tile is inside Inky's top-right quadrant.
// column and row are map tile coordinates.
bool GameManager::IsTileInsideInkyQuadrant(int column, int row) const
{
    if (column < GetMapWidth() / 2)
    {
        return false;
    }

    if (row >= GetMapHeight() / 2)
    {
        return false;
    }

    return true;
}

// Converts a world position into map tile coordinates.
// position is the world position to convert.
Vector2D GameManager::WorldToTile(Vector2D position) const
{
    float column = std::floor(position.x / (float)TileSize);
    float row = std::floor((position.y - (float)HudHeight) / (float)TileSize);
    return MakeVector2D(column, row);
}

// Converts tile coordinates into the center of that tile in world space.
// column and row are the tile coordinates.
Vector2D GameManager::TileToWorldCenter(int column, int row) const
{
    float x = (float)(column * TileSize) + TileSize * 0.5f;
    float y = (float)(HudHeight + row * TileSize) + TileSize * 0.5f;
    return MakeVector2D(x, y);
}

// Returns the next tile center where a moving object can make a turn.
// worldPosition is the current center point, and movementDirection is the current direction.
Vector2D GameManager::GetNextTurnCenter(Vector2D worldPosition, Vector2D movementDirection) const
{
    Vector2D currentTile = WorldToTile(worldPosition);
    int column = (int)currentTile.x;
    int row = (int)currentTile.y;
    Vector2D currentCenter = TileToWorldCenter(column, row);

    if (movementDirection.x > 0.0f && worldPosition.x > currentCenter.x + TURN_CENTER_TOLERANCE)
    {
        column = column + 1;
    }
    else if (movementDirection.x < 0.0f && worldPosition.x < currentCenter.x - TURN_CENTER_TOLERANCE)
    {
        column = column - 1;
    }
    else if (movementDirection.y > 0.0f && worldPosition.y > currentCenter.y + TURN_CENTER_TOLERANCE)
    {
        row = row + 1;
    }
    else if (movementDirection.y < 0.0f && worldPosition.y < currentCenter.y - TURN_CENTER_TOLERANCE)
    {
        row = row - 1;
    }

    column = WrapColumnIndex(column, GetMapWidth());
    return TileToWorldCenter(column, row);
}

// Returns true if the neighboring tile in one direction is walkable.
// worldPosition is the current world position, direction is the requested move,
// and the two booleans control ghost house rules.
bool GameManager::CanEnterNeighborTile(Vector2D worldPosition,
                                       Vector2D direction,
                                       bool ghostCanUseDoor,
                                       bool ghostCanUseHouse) const
{
    if (IsZeroVector(direction))
    {
        return false;
    }

    Vector2D tile = WorldToTile(worldPosition);
    int column = (int)tile.x + (int)direction.x;
    int row = (int)tile.y + (int)direction.y;
    column = WrapColumnIndex(column, GetMapWidth());
    return IsWalkableTile(column, row, ghostCanUseDoor, ghostCanUseHouse);
}

// Snaps an object toward the center of its current lane.
// worldPosition is the position to adjust, and direction is the current movement direction.
void GameManager::AlignToTileCenter(Vector2D& worldPosition, Vector2D direction) const
{
    Vector2D tile = WorldToTile(worldPosition);
    Vector2D center = TileToWorldCenter((int)tile.x, (int)tile.y);
    bool nearCenterX = std::fabs(center.x - worldPosition.x) <= LANE_SNAP_TOLERANCE;
    bool nearCenterY = std::fabs(center.y - worldPosition.y) <= LANE_SNAP_TOLERANCE;

    if (direction.x != 0.0f && nearCenterY)
    {
        worldPosition.y = center.y;
    }
    else if (direction.y != 0.0f && nearCenterX)
    {
        worldPosition.x = center.x;
    }
    else if (IsZeroVector(direction))
    {
        if (nearCenterX)
        {
            worldPosition.x = center.x;
        }

        if (nearCenterY)
        {
            worldPosition.y = center.y;
        }
    }
}

// Returns true if a world position is near the center of its tile.
// worldPosition is the point to test, and tolerance is the allowed distance from center.
bool GameManager::IsNearTileCenter(Vector2D worldPosition, float tolerance) const
{
    Vector2D tile = WorldToTile(worldPosition);
    Vector2D center = TileToWorldCenter((int)tile.x, (int)tile.y);
    if (std::fabs(center.x - worldPosition.x) > tolerance)
    {
        return false;
    }

    if (std::fabs(center.y - worldPosition.y) > tolerance)
    {
        return false;
    }

    return true;
}

// Wraps a world position through the left and right tunnel edges.
// worldPosition is the position to adjust.
void GameManager::WrapPosition(Vector2D& worldPosition) const
{
    float leftExit = -TileSize * 0.5f;
    float rightExit = (float)(GetMapWidth() * TileSize) - TileSize * 0.5f;
    float firstCenter = TileSize * 0.5f;
    float lastCenter = (float)(GetMapWidth() * TileSize) - TileSize * 0.5f;

    if (worldPosition.x < leftExit)
    {
        worldPosition.x = lastCenter;
    }
    else if (worldPosition.x > rightExit)
    {
        worldPosition.x = firstCenter;
    }
}

// Returns a moved world position after stepping in one direction.
// position is the start, direction is the movement direction, and distance is the step length.
Vector2D GameManager::GetMovedPosition(Vector2D position, Vector2D direction, float distance) const
{
    Vector2D candidatePosition = position;
    candidatePosition.x = candidatePosition.x + direction.x * distance;
    candidatePosition.y = candidatePosition.y + direction.y * distance;
    WrapPosition(candidatePosition);
    return candidatePosition;
}

// Returns true if the object collision area is fully on walkable tiles.
// candidatePosition is the moved center point, and the two booleans control ghost house rules.
bool GameManager::IsPositionAreaWalkable(Vector2D candidatePosition,
                                         bool ghostCanUseDoor,
                                         bool ghostCanUseHouse) const
{
    int leftTile = (int)std::floor((candidatePosition.x - COLLISION_RADIUS) / (float)TileSize);
    int rightTile = (int)std::floor((candidatePosition.x + COLLISION_RADIUS) / (float)TileSize);
    int topTile = (int)std::floor((candidatePosition.y - (float)HudHeight - COLLISION_RADIUS) / (float)TileSize);
    int bottomTile = (int)std::floor((candidatePosition.y - (float)HudHeight + COLLISION_RADIUS) / (float)TileSize);

    for (int row = topTile; row <= bottomTile; row++)
    {
        for (int column = leftTile; column <= rightTile; column++)
        {
            int wrappedColumn = WrapColumnIndex(column, GetMapWidth());
            if (!IsWalkableTile(wrappedColumn, row, ghostCanUseDoor, ghostCanUseHouse))
            {
                return false;
            }
        }
    }

    return true;
}

// Returns true if an object can move a distance in one direction.
// position is the current center point, direction is the movement direction,
// distance is the move length, and the booleans control ghost house rules.
bool GameManager::CanMove(Vector2D position,
                          Vector2D direction,
                          float distance,
                          bool ghostCanUseDoor,
                          bool ghostCanUseHouse) const
{
    if (IsZeroVector(direction))
    {
        return false;
    }

    Vector2D candidatePosition = GetMovedPosition(position, direction, distance);
    return IsPositionAreaWalkable(candidatePosition, ghostCanUseDoor, ghostCanUseHouse);
}

// Returns true if a tile is allowed for the current object.
// column and row are tile coordinates, and the booleans control ghost house rules.
bool GameManager::IsWalkableTile(int column, int row, bool ghostCanUseDoor, bool ghostCanUseHouse) const
{
    if (row < 0 || row >= GetMapHeight())
    {
        return false;
    }

    if (column < 0 || column >= GetMapWidth())
    {
        return false;
    }

    char cell = mapRows[row][column];
    if (cell == '#')
    {
        return false;
    }

    if (cell == '-')
    {
        return ghostCanUseDoor;
    }

    if (cell == ' ' && IsGhostHouseTileInternal(column, row))
    {
        return ghostCanUseHouse;
    }

    return true;
}

// Counts how many active pellets remain.
int GameManager::CountRemainingPellets() const
{
    int count = 0;
    for (int index = 0; index < (int)gameObjects.size(); index++)
    {
        GameObject* object = gameObjects[index];
        if (IsPelletObject(object))
        {
            count = count + 1;
        }
    }

    return count;
}

// Collects every valid movement direction for one ghost.
// ghost is the ghost to test, allowReverse controls reverse moves,
// and validDirections receives the allowed results.
void GameManager::CollectValidGhostDirections(const Ghost* ghost,
                                              bool allowReverse,
                                              std::vector<Vector2D>& validDirections) const
{
    Vector2D up = MakeVector2D(0.0f, -1.0f);
    Vector2D left = MakeVector2D(-1.0f, 0.0f);
    Vector2D down = MakeVector2D(0.0f, 1.0f);
    Vector2D right = MakeVector2D(1.0f, 0.0f);
    Vector2D reverseDirection = GetOppositeDirection(ghost->GetDirection());

    TryAddGhostDirection(ghost, up, reverseDirection, allowReverse, validDirections);
    TryAddGhostDirection(ghost, left, reverseDirection, allowReverse, validDirections);
    TryAddGhostDirection(ghost, down, reverseDirection, allowReverse, validDirections);
    TryAddGhostDirection(ghost, right, reverseDirection, allowReverse, validDirections);
}

// Adds one ghost direction if it is allowed by the map and reverse rules.
// ghost is the ghost to test, option is the direction to test,
// reverseDirection is the direct opposite of the current direction,
// allowReverse controls reverse moves, and validDirections stores accepted directions.
void GameManager::TryAddGhostDirection(const Ghost* ghost,
                                       Vector2D option,
                                       Vector2D reverseDirection,
                                       bool allowReverse,
                                       std::vector<Vector2D>& validDirections) const
{
    if (!allowReverse && AreVectorsEqual(option, reverseDirection))
    {
        return;
    }

    bool canUseDoor = ghost->IsEaten();
    bool canUseHouse = ghost->IsEaten();
    bool canEnterNextTile = CanEnterNeighborTile(ghost->GetPosition(), option, canUseDoor, canUseHouse);
    bool canMoveShortDistance = CanMove(ghost->GetPosition(), option, GHOST_DIRECTION_PROBE_DISTANCE,
                                        canUseDoor, canUseHouse);
    if (canEnterNextTile && canMoveShortDistance)
    {
        validDirections.push_back(option);
    }
}

// Returns a random direction from a list of valid directions.
// validDirections is the list of allowed directions.
Vector2D GameManager::ChooseRandomDirection(const std::vector<Vector2D>& validDirections) const
{
    if ((int)validDirections.size() == 1)
    {
        return validDirections[0];
    }

    int randomIndex = GetRandomValue(0, (int)validDirections.size() - 1);
    return validDirections[randomIndex];
}

// Returns the valid direction that gets closest to the target tile.
// ghost is the moving ghost, validDirections is the list of options,
// and targetWorldPosition is the world position the ghost wants to approach.
Vector2D GameManager::ChooseClosestDirection(const Ghost* ghost,
                                             const std::vector<Vector2D>& validDirections,
                                             Vector2D targetWorldPosition) const
{
    Vector2D targetTile = WorldToTile(targetWorldPosition);
    Vector2D ghostTile = WorldToTile(ghost->GetPosition());
    Vector2D bestDirection = validDirections[0];
    int bestDistance = 999999;

    for (int index = 0; index < (int)validDirections.size(); index++)
    {
        Vector2D nextTile = AddVectors(ghostTile, validDirections[index]);
        int distance = (int)(std::fabs(nextTile.x - targetTile.x) + std::fabs(nextTile.y - targetTile.y));
        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestDirection = validDirections[index];
        }
    }

    return bestDirection;
}

// Chooses the next movement direction for a ghost.
// ghost is the ghost to steer, targetWorldPosition is the target,
// randomize decides whether frightened mode should choose randomly,
// and allowReverse decides whether the reverse direction is allowed.
Vector2D GameManager::ChooseGhostDirection(const Ghost* ghost,
                                           Vector2D targetWorldPosition,
                                           bool randomize,
                                           bool allowReverse) const
{
    std::vector<Vector2D> validDirections;
    CollectValidGhostDirections(ghost, allowReverse, validDirections);

    if (validDirections.empty())
    {
        Vector2D reverseDirection = GetOppositeDirection(ghost->GetDirection());
        validDirections.push_back(reverseDirection);
    }

    if (randomize)
    {
        return ChooseRandomDirection(validDirections);
    }

    return ChooseClosestDirection(ghost, validDirections, targetWorldPosition);
}
