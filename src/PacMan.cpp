/*
    PacMan.cpp
    ----------
    This file implements the PacMan class.
    Pac-Man handles his own movement, queued turning, score, lives, and
    drawing. The GameManager gives Pac-Man map information and collision rules.
*/

#include "PacMan.h"

#include "GameManager.h"

static const float PACMAN_TURN_PROBE_DISTANCE = 2.0f;
static const float PACMAN_MOVE_SPEED = 105.0f;
static const int PACMAN_START_LIVES = 3;

// Creates a new Pac-Man object.
// owner is the GameManager, startPosition is the spawn point,
// and spriteTexture is the texture used for drawing.
PacMan::PacMan(GameManager* owner, Vector2D startPosition, Texture2D* spriteTexture)
    : GameObject(TAG_PACMAN)
{
    manager = owner;
    texture = spriteTexture;
    spawnPosition = startPosition;
    currentDirection = MakeVector2D(-1.0f, 0.0f);
    nextDirection = MakeVector2D(-1.0f, 0.0f);
    hasQueuedDirection = false;
    moveSpeed = PACMAN_MOVE_SPEED;
    lives = PACMAN_START_LIVES;
    score = 0;
    ghostCombo = 0;
    position = startPosition;
}

// Stores the next requested direction for Pac-Man.
// direction is the new requested direction.
void PacMan::QueueDirection(Vector2D direction)
{
    nextDirection = direction;
    hasQueuedDirection = true;
}

// Returns true if two directions point in opposite ways.
// firstDirection and secondDirection are the directions to compare.
bool PacMan::IsOppositeDirection(Vector2D firstDirection, Vector2D secondDirection) const
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

// Returns the drawing rotation based on the current movement direction.
float PacMan::GetRotationAngle() const
{
    if (currentDirection.x > 0.0f)
    {
        return 0.0f;
    }

    if (currentDirection.x < 0.0f)
    {
        return 180.0f;
    }

    if (currentDirection.y < 0.0f)
    {
        return 270.0f;
    }

    if (currentDirection.y > 0.0f)
    {
        return 90.0f;
    }

    return 180.0f;
}

// Applies an immediate reverse if the queued direction is exactly opposite.
void PacMan::TryImmediateReverse()
{
    if (!hasQueuedDirection)
    {
        return;
    }

    if (!IsOppositeDirection(currentDirection, nextDirection))
    {
        return;
    }

    currentDirection = nextDirection;
    hasQueuedDirection = false;
}

// Tries to use the queued direction when Pac-Man reaches a tile center.
void PacMan::TryQueuedTurn()
{
    if (!hasQueuedDirection)
    {
        return;
    }

    if (!manager->IsNearTileCenter(position, 1.0f))
    {
        return;
    }

    Vector2D tileCenter = position;
    manager->AlignToTileCenter(tileCenter, currentDirection);
    bool canEnterNextTile = manager->CanEnterNeighborTile(tileCenter, nextDirection, false, false);
    bool canMoveIntoTurn = manager->CanMove(tileCenter, nextDirection, PACMAN_TURN_PROBE_DISTANCE, false, false);
    if (canEnterNextTile && canMoveIntoTurn)
    {
        position = tileCenter;
        currentDirection = nextDirection;
    }

    hasQueuedDirection = false;
}

// Moves Pac-Man forward if the path is clear.
// distance is how far Pac-Man should move this frame.
void PacMan::MoveForward(float distance)
{
    bool atTileCenter = manager->IsNearTileCenter(position, 1.0f);
    bool canLeaveCurrentTile = true;
    if (atTileCenter)
    {
        canLeaveCurrentTile = manager->CanEnterNeighborTile(position, currentDirection, false, false);
    }

    if (canLeaveCurrentTile && manager->CanMove(position, currentDirection, distance, false, false))
    {
        position.x = position.x + currentDirection.x * distance;
        position.y = position.y + currentDirection.y * distance;
        manager->WrapPosition(position);
        return;
    }

    StopAtWall();
}

// Snaps Pac-Man back to the lane center when movement is blocked.
void PacMan::StopAtWall()
{
    manager->AlignToTileCenter(position, currentDirection);
}

// Returns Pac-Man's current movement direction.
Vector2D PacMan::GetDirection() const
{
    return currentDirection;
}

// Returns the number of lives Pac-Man still has.
int PacMan::GetLives() const
{
    return lives;
}

// Returns the current score.
int PacMan::GetScore() const
{
    return score;
}

// Returns the current ghost combo count.
int PacMan::GetGhostCombo() const
{
    return ghostCombo;
}

// Adds points to the score.
// value is the number of points to add.
void PacMan::AddScore(int value)
{
    score = score + value;
}

// Resets the ghost combo count back to zero.
void PacMan::ResetGhostCombo()
{
    ghostCombo = 0;
}

// Increases the ghost combo count by one.
void PacMan::IncreaseGhostCombo()
{
    ghostCombo = ghostCombo + 1;
}

// Removes one life if Pac-Man still has any left.
void PacMan::LoseLife()
{
    if (lives > 0)
    {
        lives = lives - 1;
    }
}

// Sends Pac-Man back to his starting position and direction.
void PacMan::ResetPosition()
{
    position = spawnPosition;
    currentDirection = MakeVector2D(-1.0f, 0.0f);
    nextDirection = currentDirection;
    hasQueuedDirection = false;
}

// Draws Pac-Man with the correct rotation.
void PacMan::Render()
{
    Rectangle source;
    source.x = 0.0f;
    source.y = 0.0f;
    source.width = 16.0f;
    source.height = 16.0f;

    float size = (float)manager->GetTileSize();
    Rectangle destination;
    destination.x = position.x;
    destination.y = position.y;
    destination.width = size;
    destination.height = size;

    Vector2 origin;
    origin.x = size * 0.5f;
    origin.y = size * 0.5f;
    float rotation = GetRotationAngle();
    DrawTexturePro(*texture, source, destination, origin, rotation, WHITE);
}

// Updates Pac-Man for one frame.
void PacMan::Update()
{
    float distance = moveSpeed * manager->GetDeltaTime();
    TryImmediateReverse();
    TryQueuedTurn();
    MoveForward(distance);
}
