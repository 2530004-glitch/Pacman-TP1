#pragma once

#include "GameObject.h"
#include "raylib.h"

// A ghost in the maze. Each ghost type has its own AI targeting logic.
// Ghosts can chase, become frightened (blue), or return home after being eaten.
class GameManager;

class Ghost : public GameObject
{
private:
    GameManager* manager;
    Texture2D* chaseTexture;
    Texture2D* frightenedTexture;
    GhostType  ghostType;
    Vector2D   spawnPosition;
    Vector2D   currentDirection;
    Vector2D   patrolTarget;
    float      moveSpeed;
    GhostState state;
    bool       allowReverse;
    float      releaseTimer;
    float      bounceTimer;
    bool       waitingForRelease;
    bool       exitingHouse;

    float    GetReleaseDelay() const;
    int      GetDoorColumn() const;
    int      GetMazeExitColumn() const;
    float    GetCurrentSpeed() const;
    Vector2D GetChaseTarget() const;
    Vector2D GetCurrentExitTarget() const;

    void UpdateWaitingInsideHouse(float deltaTime);
    void StartLeavingHouse();
    void UpdateExitFromHouse(float distance);
    void MoveTowardPoint(Vector2D target, float distance);
    void MoveThroughMaze(float distance);
    void FinishEatenTripIfNeeded();

public:
    Ghost(GameManager* owner,
          GhostType type,
          Vector2D startPosition,
          Vector2D defaultPatrolTarget,
          Texture2D* normalTexture,
          Texture2D* blueTexture);

    GhostType GetGhostType() const;
    Vector2D  GetDirection() const;
    bool      IsFrightened() const;
    bool      IsEaten() const;
    void      SetFrightened();
    void      SetEaten();
    void      SetChase();
    void      ResetPosition();

    void Render();
    void Update();
};
