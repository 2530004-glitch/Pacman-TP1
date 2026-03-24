#pragma once

#include "GameObject.h"
#include "raylib.h"

/*
    PacMan.h
    --------
    This file defines the PacMan class.
    PacMan inherits from GameObject and stores movement, lives, score,
    queued turning, and drawing data. The GameManager updates Pac-Man
    during gameplay and checks his collisions with pellets and ghosts.
*/

class GameManager;

class PacMan : public GameObject
{
private:
    GameManager* manager;
    Texture2D* texture;
    Vector2D spawnPosition;
    Vector2D currentDirection;
    Vector2D nextDirection;
    bool hasQueuedDirection;
    float moveSpeed;
    int lives;
    int score;
    int ghostCombo;

    bool IsOppositeDirection(Vector2D firstDirection, Vector2D secondDirection) const;
    float GetRotationAngle() const;
    void TryImmediateReverse();
    void TryQueuedTurn();
    void MoveForward(float distance);
    void StopAtWall();

public:
    PacMan(GameManager* owner, Vector2D startPosition, Texture2D* spriteTexture);

    void QueueDirection(Vector2D direction);
    Vector2D GetDirection() const;
    int GetLives() const;
    int GetScore() const;
    int GetGhostCombo() const;
    void AddScore(int value);
    void ResetGhostCombo();
    void IncreaseGhostCombo();
    void LoseLife();
    void ResetPosition();

    void Render();
    void Update();
};
