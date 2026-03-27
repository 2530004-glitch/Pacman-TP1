#pragma once

#include "raylib.h"

class PacMan
{
private:
    Vector2 position;
    Vector2 direction;
    Vector2 nextDirection;

    float speed;
    Texture2D texture;

    int score;
    int lives;

public:
    PacMan(Vector2 startPosition, Texture2D tex, int initialLives = 3);

    Vector2 GetPosition() const { 
        return position; 
    }
    void SetPosition(Vector2 newPosition) { 
        position = newPosition; 
    }

    int GetScore() const { return score; }
    void AddScore(int points) { 
        score += points; 
    }

    int GetLives() const { 
        return lives; 
    }

    void LoseLife() { 
        lives--; 
    }

    void Update();
    void Draw();

private:
    void HandleInput();
    void Move(float deltaTime);
    float GetRotation() const;
    void WrapAroundScreen();
};