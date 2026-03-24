#pragma once

#include "GameObject.h"
#include "raylib.h"

/*
    Ghost.h
    -------
    This file defines the Ghost class.
    A Ghost inherits from GameObject and stores its type, movement state,
    release timer, chase target logic, and drawing data. The GameManager
    creates three Ghost objects and updates them each frame.
*/

class GameManager;

class Ghost : public GameObject
{
private:
    GameManager* manager;
    Texture2D* chaseTexture;
    Texture2D* frightenedTexture;
    GhostType ghostType;
    Vector2D spawnPosition;
    Vector2D currentDirection;
    Vector2D patrolTarget;
    float moveSpeed;
    GhostState state;
    bool allowReverse;
    float releaseTimer;
    float bounceTimer;
    bool waitingForRelease;
    bool exitingHouse;

    Vector2D GetChaseTarget() const;
    Vector2D GetPinkyTarget(Vector2D pacTile, Vector2D pacDirection) const;
    Vector2D GetInkyTarget(Vector2D pacTile) const;
    Vector2D GetClydeTarget(Vector2D pacTile) const;
    float GetReleaseDelay() const;
    int GetDoorColumn() const;
    int GetMazeExitColumn() const;
    float GetCurrentSpeed() const;
    Vector2D GetDoorCenter() const;
    Vector2D GetHallwayCenter() const;
    Vector2D GetMazeExitCenter() const;
    Vector2D GetCurrentExitTarget() const;
    void UpdateWaitingInsideHouse(float deltaTime);
    void StartLeavingHouse();
    void UpdateExitFromHouse(float distance);
    void MoveTowardPoint(Vector2D target, float distance);
    void ChooseDirectionAtIntersection();
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
    Vector2D GetDirection() const;
    bool IsFrightened() const;
    bool IsEaten() const;
    void SetFrightened();
    void SetEaten();
    void SetChase();
    void ResetPosition();

    void Render();
    void Update();
};
