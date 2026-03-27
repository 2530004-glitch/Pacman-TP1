#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "ObjectTag.h"
#include "Vector2D.h"
#include "raylib.h"

// The GameManager owns the map, all gameplay objects, and runs the game loop each frame.
// It handles Pac-Man's input, ghost AI, pellet/ghost collisions, frightened mode, and drawing.
class Game;
class GameObject;
class Ghost;
class PacMan;

class GameManager
{
private:
    Game& game;
    std::vector<GameObject*> gameObjects;
    std::vector<std::string> mapRows;
    PacMan* pacMan;
    std::vector<Ghost*> ghosts;
    int currentLives;

    Texture2D* pacManTexture;
    Texture2D* coinTexture;
    Texture2D* bigCoinTexture;
    Texture2D* inkyTexture;
    Texture2D* pinkyTexture;
    Texture2D* clydeTexture;
    Texture2D* frightenedTexture;
    Texture2D* wallTexture;

    float frightenedTimer;

    void LoadMap();
    void ClearObjects();
    void SpawnPelletsFromMap();
    void SpawnActors();
    void HandleInput();
    void HandlePelletCollisions();
    void HandleGhostCollisions();
    void ActivateFrightenedMode();
    void UpdateFrightenedMode();
    void DrawMaze() const;
    void DrawHud() const;

    bool     IsSameTile(Vector2D a, Vector2D b) const;
    bool     IsPositionWalkable(Vector2D pos, bool useDoor, bool useHouse) const;

public:
    static const int TileSize  = 32;
    static const int HudHeight = 96;

    GameManager(Game& owner);
    ~GameManager();

    void AddObject(GameObject* object);
    void RemoveObject(GameObject* object);
    void UpdateGame();
    void RenderGame() const;
    void ResetLevel();
    void ResetActors();

    void SetTextures(
        Texture2D* pacTexture,
        Texture2D* normalPelletTexture,
        Texture2D* superPelletTexture,
        Texture2D* greenGhostTexture,
        Texture2D* yellowGhostTexture,
        Texture2D* orangeGhostTexture,
        Texture2D* blueGhostTexture,
        Texture2D* mazeTexture);

    float    GetDeltaTime() const;
    int      GetMapWidth() const;
    int      GetMapHeight() const;
    PacMan*  GetPacMan() const;
    Vector2D GetGhostHouseTarget() const;
    bool     IsTileInsideInkyQuadrant(int column, int row) const;
    Vector2D WorldToTile(Vector2D position) const;
    Vector2D TileToWorldCenter(int column, int row) const;
    Vector2D GetNextTurnCenter(Vector2D worldPosition, Vector2D movementDirection) const;
    bool     CanEnterNeighborTile(Vector2D worldPosition, Vector2D direction, bool useDoor, bool useHouse) const;
    void     AlignToTileCenter(Vector2D& worldPosition, Vector2D direction) const;
    bool     IsNearTileCenter(Vector2D worldPosition, float tolerance) const;
    void     WrapPosition(Vector2D& worldPosition) const;
    bool     CanMove(Vector2D position, Vector2D direction, float distance, bool useDoor, bool useHouse) const;
    bool     IsWalkableTile(int column, int row, bool useDoor, bool useHouse) const;
    int      CountRemainingPellets() const;
    Vector2D ChooseGhostDirection(const Ghost* ghost, Vector2D target, bool randomize, bool allowReverse) const;
};
