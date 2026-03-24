/*
    Ghost.cpp
    ---------
    This file implements the Ghost class.
    Each ghost can wait in the house, leave the house, chase Pac-Man,
    become frightened, or return home after being eaten.
*/

#include "Ghost.h"

#include <cmath>

#include "GameManager.h"
#include "PacMan.h"

static const float GHOST_MOVE_SPEED = 92.0f;
static const float GHOST_BOUNCE_DISTANCE = 6.0f;
static const float GHOST_BOUNCE_SPEED = 3.0f;
static const float GHOST_ALIGNMENT_TOLERANCE = 0.5f;
static const float GHOST_FRIGHTENED_SPEED_MULTIPLIER = 0.5f;
static const float GHOST_EATEN_SPEED_MULTIPLIER = 1.7f;
static const int GHOST_DOOR_COLUMN = 13;
static const int GHOST_DOOR_ROW = 12;
static const int GHOST_HALLWAY_ROW = 11;
static const int GHOST_MAZE_EXIT_ROW = 10;

// Creates a new ghost.
// owner is the GameManager, type selects the ghost AI,
// startPosition is the spawn point, defaultPatrolTarget is the ghost's patrol goal,
// normalTexture is the chase sprite, and blueTexture is the frightened sprite.
Ghost::Ghost(GameManager* owner,
             GhostType type,
             Vector2D startPosition,
             Vector2D defaultPatrolTarget,
             Texture2D* normalTexture,
             Texture2D* blueTexture)
    : GameObject(TAG_GHOST)
{
    manager = owner;
    chaseTexture = normalTexture;
    frightenedTexture = blueTexture;
    ghostType = type;
    spawnPosition = startPosition;
    currentDirection = MakeVector2D(-1.0f, 0.0f);
    patrolTarget = defaultPatrolTarget;
    moveSpeed = GHOST_MOVE_SPEED;
    state = GHOST_STATE_CHASE;
    allowReverse = false;
    releaseTimer = 0.0f;
    bounceTimer = 0.0f;
    waitingForRelease = false;
    exitingHouse = false;
    ResetPosition();
}

// Returns the ghost type.
GhostType Ghost::GetGhostType() const
{
    return ghostType;
}

// Returns the current movement direction.
Vector2D Ghost::GetDirection() const
{
    return currentDirection;
}

// Returns true if the ghost is frightened.
bool Ghost::IsFrightened() const
{
    return state == GHOST_STATE_FRIGHTENED;
}

// Returns true if the ghost is currently eaten.
bool Ghost::IsEaten() const
{
    return state == GHOST_STATE_EATEN;
}

// Returns the release delay for this ghost type.
float Ghost::GetReleaseDelay() const
{
    if (ghostType == GHOST_PINKY)
    {
        return 0.0f;
    }

    if (ghostType == GHOST_INKY)
    {
        return 5.0f;
    }

    return 10.0f;
}

// Returns the door column this ghost uses while leaving the house.
int Ghost::GetDoorColumn() const
{
    if (ghostType == GHOST_CLYDE)
    {
        return 14;
    }

    return GHOST_DOOR_COLUMN;
}

// Returns the maze exit column this ghost uses after passing the door.
int Ghost::GetMazeExitColumn() const
{
    if (ghostType == GHOST_CLYDE)
    {
        return 15;
    }

    return 12;
}

// Returns the current move speed based on the ghost state.
float Ghost::GetCurrentSpeed() const
{
    if (state == GHOST_STATE_FRIGHTENED)
    {
        return moveSpeed * GHOST_FRIGHTENED_SPEED_MULTIPLIER;
    }

    if (state == GHOST_STATE_EATEN)
    {
        return moveSpeed * GHOST_EATEN_SPEED_MULTIPLIER;
    }

    return moveSpeed;
}

// Returns the world position of the ghost house door center.
Vector2D Ghost::GetDoorCenter() const
{
    return manager->TileToWorldCenter(GetDoorColumn(), GHOST_DOOR_ROW);
}

// Returns the world position of the hallway point above the house door.
Vector2D Ghost::GetHallwayCenter() const
{
    return manager->TileToWorldCenter(GetMazeExitColumn(), GHOST_HALLWAY_ROW);
}

// Returns the world position where the ghost fully enters the maze.
Vector2D Ghost::GetMazeExitCenter() const
{
    return manager->TileToWorldCenter(GetMazeExitColumn(), GHOST_MAZE_EXIT_ROW);
}

// Returns Pinky's chase target.
// pacTile is Pac-Man's tile, and pacDirection is Pac-Man's movement direction.
Vector2D Ghost::GetPinkyTarget(Vector2D pacTile, Vector2D pacDirection) const
{
    int targetColumn = (int)(pacTile.x + pacDirection.x * 4.0f);
    int targetRow = (int)(pacTile.y + pacDirection.y * 4.0f);
    return manager->TileToWorldCenter(targetColumn, targetRow);
}

// Returns Inky's chase or patrol target.
// pacTile is Pac-Man's current tile.
Vector2D Ghost::GetInkyTarget(Vector2D pacTile) const
{
    bool pacManInQuadrant = manager->IsTileInsideInkyQuadrant((int)pacTile.x, (int)pacTile.y);
    if (pacManInQuadrant)
    {
        return manager->TileToWorldCenter((int)pacTile.x, (int)pacTile.y);
    }

    return patrolTarget;
}

// Returns Clyde's chase or retreat target.
// pacTile is Pac-Man's current tile.
Vector2D Ghost::GetClydeTarget(Vector2D pacTile) const
{
    Vector2D ghostTile = manager->WorldToTile(position);
    int distance = (int)(std::fabs(ghostTile.x - pacTile.x) + std::fabs(ghostTile.y - pacTile.y));
    if (distance > 8)
    {
        return manager->TileToWorldCenter((int)pacTile.x, (int)pacTile.y);
    }

    return patrolTarget;
}

// Returns the current chase target for this ghost.
Vector2D Ghost::GetChaseTarget() const
{
    PacMan* pacMan = manager->GetPacMan();
    if (pacMan == NULL)
    {
        return patrolTarget;
    }

    if (state == GHOST_STATE_EATEN)
    {
        return manager->GetGhostHouseTarget();
    }

    Vector2D pacTile = manager->WorldToTile(pacMan->GetPosition());
    Vector2D pacDirection = pacMan->GetDirection();
    if (ghostType == GHOST_PINKY)
    {
        return GetPinkyTarget(pacTile, pacDirection);
    }

    if (ghostType == GHOST_INKY)
    {
        return GetInkyTarget(pacTile);
    }

    return GetClydeTarget(pacTile);
}

// Puts the ghost into frightened mode unless it is already eaten.
void Ghost::SetFrightened()
{
    if (state != GHOST_STATE_EATEN)
    {
        state = GHOST_STATE_FRIGHTENED;
        allowReverse = false;
    }
}

// Puts the ghost into eaten mode.
void Ghost::SetEaten()
{
    state = GHOST_STATE_EATEN;
    allowReverse = true;
    waitingForRelease = false;
    exitingHouse = false;
}

// Puts the ghost back into chase mode.
void Ghost::SetChase()
{
    state = GHOST_STATE_CHASE;
    allowReverse = false;
}

// Sends the ghost back to its original spawn state.
void Ghost::ResetPosition()
{
    position = spawnPosition;
    currentDirection = MakeVector2D(0.0f, -1.0f);
    state = GHOST_STATE_CHASE;
    allowReverse = false;
    releaseTimer = GetReleaseDelay();
    bounceTimer = 0.0f;
    waitingForRelease = releaseTimer > 0.0f;
    exitingHouse = releaseTimer <= 0.0f;
}

// Updates the waiting bounce animation inside the ghost house.
// deltaTime is the frame time in seconds.
void Ghost::UpdateWaitingInsideHouse(float deltaTime)
{
    releaseTimer = releaseTimer - deltaTime;
    bounceTimer = bounceTimer + deltaTime * GHOST_BOUNCE_SPEED;
    position.x = spawnPosition.x;
    position.y = spawnPosition.y + std::sin(bounceTimer) * GHOST_BOUNCE_DISTANCE;
    if (releaseTimer <= 0.0f)
    {
        StartLeavingHouse();
    }
}

// Starts the house exit movement.
void Ghost::StartLeavingHouse()
{
    waitingForRelease = false;
    exitingHouse = true;
    position = spawnPosition;
    currentDirection = MakeVector2D(0.0f, -1.0f);
}

// Returns the current target point for the exit path out of the house.
Vector2D Ghost::GetCurrentExitTarget() const
{
    Vector2D doorCenter = GetDoorCenter();
    Vector2D hallwayCenter = GetHallwayCenter();
    Vector2D mazeExitCenter = GetMazeExitCenter();
    if (std::fabs(position.x - doorCenter.x) > GHOST_ALIGNMENT_TOLERANCE &&
        position.y > hallwayCenter.y + GHOST_ALIGNMENT_TOLERANCE)
    {
        return MakeVector2D(doorCenter.x, position.y);
    }

    if (position.y > hallwayCenter.y + GHOST_ALIGNMENT_TOLERANCE)
    {
        return MakeVector2D(doorCenter.x, hallwayCenter.y);
    }

    if (std::fabs(position.x - hallwayCenter.x) > GHOST_ALIGNMENT_TOLERANCE)
    {
        return hallwayCenter;
    }

    return mazeExitCenter;
}

// Moves the ghost toward a target point without passing it.
// target is the world point to approach, and distance is the movement length.
void Ghost::MoveTowardPoint(Vector2D target, float distance)
{
    if (std::fabs(position.x - target.x) > GHOST_ALIGNMENT_TOLERANCE)
    {
        if (position.x < target.x)
        {
            currentDirection = MakeVector2D(1.0f, 0.0f);
        }
        else
        {
            currentDirection = MakeVector2D(-1.0f, 0.0f);
        }

        float step = distance;
        if (step > std::fabs(position.x - target.x))
        {
            step = std::fabs(position.x - target.x);
        }

        position.x = position.x + currentDirection.x * step;
        return;
    }

    if (std::fabs(position.y - target.y) > GHOST_ALIGNMENT_TOLERANCE)
    {
        if (position.y < target.y)
        {
            currentDirection = MakeVector2D(0.0f, 1.0f);
        }
        else
        {
            currentDirection = MakeVector2D(0.0f, -1.0f);
        }

        float step = distance;
        if (step > std::fabs(position.y - target.y))
        {
            step = std::fabs(position.y - target.y);
        }

        position.y = position.y + currentDirection.y * step;
        return;
    }

    position = target;
}

// Updates the ghost while it is leaving the ghost house.
// distance is the movement length for this frame.
void Ghost::UpdateExitFromHouse(float distance)
{
    Vector2D target = GetCurrentExitTarget();
    MoveTowardPoint(target, distance);
    if (position.y <= GetMazeExitCenter().y + GHOST_ALIGNMENT_TOLERANCE)
    {
        exitingHouse = false;
        position = GetMazeExitCenter();
        SetChase();
    }
}

// Chooses a new direction at an intersection.
void Ghost::ChooseDirectionAtIntersection()
{
    Vector2D target = GetChaseTarget();
    Vector2D nextDirection = manager->ChooseGhostDirection(this, target, IsFrightened(), allowReverse);
    if (!IsZeroVector(nextDirection))
    {
        currentDirection = nextDirection;
    }

    allowReverse = false;
}

// Moves the ghost through the normal maze.
// distance is the movement length for this frame.
void Ghost::MoveThroughMaze(float distance)
{
    Vector2D centeredPosition = position;
    manager->AlignToTileCenter(centeredPosition, currentDirection);
    position = centeredPosition;

    bool atIntersection = manager->IsNearTileCenter(position, 1.0f);
    if (atIntersection)
    {
        ChooseDirectionAtIntersection();
    }

    bool canUseDoor = state == GHOST_STATE_EATEN;
    bool canUseHouse = state == GHOST_STATE_EATEN;
    bool canLeaveCurrentTile = true;
    if (atIntersection)
    {
        canLeaveCurrentTile = manager->CanEnterNeighborTile(position, currentDirection, canUseDoor, canUseHouse);
    }

    if (canLeaveCurrentTile && manager->CanMove(position, currentDirection, distance, canUseDoor, canUseHouse))
    {
        position.x = position.x + currentDirection.x * distance;
        position.y = position.y + currentDirection.y * distance;
        manager->WrapPosition(position);
    }
    else if (atIntersection)
    {
        position = centeredPosition;
    }
}

// Checks whether an eaten ghost reached home and should leave again.
void Ghost::FinishEatenTripIfNeeded()
{
    if (state != GHOST_STATE_EATEN)
    {
        return;
    }

    Vector2D currentTile = manager->WorldToTile(position);
    Vector2D homeTile = manager->WorldToTile(manager->GetGhostHouseTarget());
    if (AreVectorsEqual(currentTile, homeTile))
    {
        position = manager->GetGhostHouseTarget();
        SetChase();
        exitingHouse = true;
        currentDirection = MakeVector2D(0.0f, -1.0f);
    }
}

// Draws the ghost sprite unless the ghost is currently in eaten mode.
void Ghost::Render()
{
    if (state == GHOST_STATE_EATEN)
    {
        return;
    }

    Texture2D* textureToDraw = chaseTexture;
    if (IsFrightened())
    {
        textureToDraw = frightenedTexture;
    }

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
    DrawTexturePro(*textureToDraw, source, destination, origin, 0.0f, WHITE);
}

// Updates the ghost for one frame.
void Ghost::Update()
{
    float deltaTime = manager->GetDeltaTime();
    if (waitingForRelease)
    {
        UpdateWaitingInsideHouse(deltaTime);
        return;
    }

    float distance = GetCurrentSpeed() * deltaTime;
    if (exitingHouse)
    {
        UpdateExitFromHouse(distance);
        return;
    }

    MoveThroughMaze(distance);
    FinishEatenTripIfNeeded();
}
