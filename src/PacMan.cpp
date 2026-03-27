#include "PacMan.h"

PacMan::PacMan(Vector2 startPosition, Texture2D tex, int initialLives)
{
    position = startPosition;
    texture = tex;

    direction = {0, 0};
    nextDirection = {0, 0};

    speed = 100.0f;

    score = 0;
    lives = initialLives;
}

// Keyboard input
void PacMan::HandleInput()
{
    if (IsKeyPressed(KEY_UP))
    {
        nextDirection = {0, -1};
    }
    if (IsKeyPressed(KEY_DOWN))
    {
        nextDirection = {0, 1};
    }
    if (IsKeyPressed(KEY_LEFT))
    {
        nextDirection = {-1, 0};
    }
    if (IsKeyPressed(KEY_RIGHT))
    {
        nextDirection = {1, 0};
    }
}

// Move PacMan
void PacMan::Move(float deltaTime)
{
    if (nextDirection.x != 0 || nextDirection.y != 0)
    {
        direction = nextDirection;
        nextDirection = {0, 0};
    }

    // Move based on direction
    position.x += direction.x * speed * deltaTime;
    position.y += direction.y * speed * deltaTime;
}

void PacMan::WrapAroundScreen()
{
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (position.x < 0) position.x = (float)screenWidth;
    if (position.x > screenWidth) position.x = 0;

    if (position.y < 0) position.y = (float)screenHeight;
    if (position.y > screenHeight) position.y = 0;
}

// Get rotation based on direction
float PacMan::GetRotation() const
{
    if (direction.x > 0) 
    {
        return 0.0f;
    }
    if (direction.x < 0) 
    {
        return 180.0f;
    }
    if (direction.y < 0) 
    {
        return 270.0f;
    }
    if (direction.y > 0) 
    {
        return 90.0f;
    }

    return 0.0f;
}

// Update
void PacMan::Update()
{
    float deltaTime = GetFrameTime();

    HandleInput();
    Move(deltaTime);
    WrapAroundScreen();
}

// Draw
void PacMan::Draw()
{
    float size = 32.0f;

    Rectangle source = {0, 0, 16, 16};
    Rectangle move = {position.x, position.y, size, size};
    Vector2 origin = {size / 2, size / 2};

    DrawTexturePro(texture, source, move, origin, GetRotation(), WHITE);
}