#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "ObjectTag.h"
#include "Vector2D.h"
#include "raylib.h"

/*
    GameManager.h
    -------------
    This file defines the GameManager class.
    The GameManager owns the map data and every gameplay object in the maze.
    It updates Pac-Man, ghosts, pellets, collisions, frightened mode, and HUD
    drawing while the main Game class handles menus and screen changes.
*/

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
    void ReadMapRows(std::ifstream& file);
    void ValidateMapSize() const;
    void ClearObjects();

    void SpawnPelletsFromMap();
    void SpawnPelletsInRow(int row);
    void CreatePelletFromCell(int column, int row, char cell);
    void SpawnActors();
    void SpawnPacMan();
    void SpawnGhosts();
    void CreateGhost(
        GhostType ghostType,
        int spawnColumn,
        int spawnRow,
        int targetColumn,
        int targetRow,
        Texture2D* chaseTexture);

    void HandleInput();
    bool TryGetPacManInput(Vector2D* inputDirection) const;
    void ApplyPacManInput(Vector2D inputDirection);
    void HandlePelletCollisions();
    void HandleGhostCollisions();
    void ActivateFrightenedMode();
    void UpdateFrightenedMode();
    void EndFrightenedMode();

    void UpdateObjects();
    void RenderObjects() const;
    void DrawMaze() const;
    void DrawMazeRow(int row) const;
    void DrawWallTile(int column, int row) const;
    void DrawHud() const;
    void DrawScoreText() const;
    void DrawLivesText() const;

    bool IsPelletObject(GameObject* object) const;
    bool IsSameTile(Vector2D firstPosition, Vector2D secondPosition) const;
    void RemoveGhostPointer(Ghost* ghost);
    Vector2D GetMovedPosition(Vector2D position, Vector2D direction, float distance) const;
    bool IsPositionAreaWalkable(Vector2D candidatePosition, bool ghostCanUseDoor, bool ghostCanUseHouse) const;
    void CollectValidGhostDirections(
        const Ghost* ghost,
        bool allowReverse,
        std::vector<Vector2D>& validDirections) 
        const;
    void TryAddGhostDirection( 
        const Ghost* ghost,
        Vector2D option,
        Vector2D reverseDirection,
        bool allowReverse,
        std::vector<Vector2D>& validDirections) 
        const;
    Vector2D ChooseRandomDirection(
        const std::vector<Vector2D>& validDirections) const;
    Vector2D ChooseClosestDirection(
        const Ghost* ghost,
        const std::vector<Vector2D>& validDirections,
        Vector2D targetWorldPosition) const;

public:
    static const int TileSize = 32;
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

    float GetDeltaTime() const;
    int GetTileSize() const;
    int GetHudHeight() const;
    int GetMapWidth() const;
    int GetMapHeight() const;
    PacMan* GetPacMan() const;
    Vector2D GetGhostHouseTarget() const;
    bool IsTileInsideInkyQuadrant(int column, int row) const;
    Vector2D WorldToTile(Vector2D position) const;
    Vector2D TileToWorldCenter(int column, int row) const;
    Vector2D GetNextTurnCenter(Vector2D worldPosition, Vector2D movementDirection) const;
    bool CanEnterNeighborTile(Vector2D worldPosition,
                              Vector2D direction,
                              bool ghostCanUseDoor,
                              bool ghostCanUseHouse) const;
    void AlignToTileCenter(Vector2D& worldPosition, Vector2D direction) const;
    bool IsNearTileCenter(Vector2D worldPosition, float tolerance) const;
    void WrapPosition(Vector2D& worldPosition) const;
    bool CanMove(Vector2D position,
                 Vector2D direction,
                 float distance,
                 bool ghostCanUseDoor,
                 bool ghostCanUseHouse) const;
    bool IsWalkableTile(int column, int row, bool ghostCanUseDoor, bool ghostCanUseHouse) const;
    int CountRemainingPellets() const;
    Vector2D ChooseGhostDirection(const Ghost* ghost,
                                  Vector2D targetWorldPosition,
                                  bool randomize,
                                  bool allowReverse) const;
};
