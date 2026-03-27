#include "Ghost.h"

#include <cmath>

#include "GameManager.h"
#include "PacMan.h"

static const float MOVE_SPEED             = 92.0f;
static const float BOUNCE_DISTANCE        = 6.0f;
static const float BOUNCE_SPEED           = 3.0f;
static const float ALIGN_TOLERANCE        = 0.5f;
static const float FRIGHTENED_SPEED_MULT  = 0.5f;
static const float EATEN_SPEED_MULT       = 1.7f;
static const int   DOOR_ROW               = 12;
static const int   HALLWAY_ROW            = 11;
static const int   MAZE_EXIT_ROW          = 10;

Ghost::Ghost(GameManager* owner,
             GhostType type,
             Vector2D startPosition,
             Vector2D defaultPatrolTarget,
             Texture2D* normalTexture,
             Texture2D* blueTexture)
    : GameObject(TAG_GHOST)
{
    manager          = owner;
    chaseTexture     = normalTexture;
    frightenedTexture = blueTexture;
    ghostType        = type;
    spawnPosition    = startPosition;
    patrolTarget     = defaultPatrolTarget;
    currentDirection = MakeVector2D(-1.0f, 0.0f);
    moveSpeed        = MOVE_SPEED;
    state            = GHOST_STATE_CHASE;
    allowReverse     = false;
    releaseTimer     = 0.0f;
    bounceTimer      = 0.0f;
    waitingForRelease = false;
    exitingHouse     = false;
    ResetPosition();
}

GhostType Ghost::GetGhostType() const { return ghostType; }
Vector2D  Ghost::GetDirection()  const { return currentDirection; }
bool      Ghost::IsFrightened()  const { return state == GHOST_STATE_FRIGHTENED; }
bool      Ghost::IsEaten()       const { return state == GHOST_STATE_EATEN; }

// How long this ghost waits in the house before leaving.
float Ghost::GetReleaseDelay() const
{
    if (ghostType == GHOST_PINKY) return 0.0f;
    if (ghostType == GHOST_INKY)  return 5.0f;
    return 10.0f; // Clyde
}

// Clyde exits from a slightly different door column than the others.
int Ghost::GetDoorColumn() const
{
    return (ghostType == GHOST_CLYDE) ? 14 : 13;
}

int Ghost::GetMazeExitColumn() const
{
    return (ghostType == GHOST_CLYDE) ? 15 : 12;
}

float Ghost::GetCurrentSpeed() const
{
    if (state == GHOST_STATE_FRIGHTENED) return moveSpeed * FRIGHTENED_SPEED_MULT;
    if (state == GHOST_STATE_EATEN)      return moveSpeed * EATEN_SPEED_MULT;
    return moveSpeed;
}

// Returns what world position this ghost is currently aiming for.
Vector2D Ghost::GetChaseTarget() const
{
    // Simplified chase target without relying on PacMan getters.
    if (state == GHOST_STATE_EATEN)
        return manager->GetGhostHouseTarget();

    // Default behavior: head toward the patrol target.
    return patrolTarget;
}

void Ghost::SetFrightened()
{
    if (state != GHOST_STATE_EATEN)
    {
        state        = GHOST_STATE_FRIGHTENED;
        allowReverse = false;
    }
}

void Ghost::SetEaten()
{
    state             = GHOST_STATE_EATEN;
    allowReverse      = true;
    waitingForRelease = false;
    exitingHouse      = false;
}

void Ghost::SetChase()
{
    state        = GHOST_STATE_CHASE;
    allowReverse = false;
}

void Ghost::ResetPosition()
{
    position          = spawnPosition;
    currentDirection  = MakeVector2D(0.0f, -1.0f);
    state             = GHOST_STATE_CHASE;
    allowReverse      = false;
    releaseTimer      = GetReleaseDelay();
    bounceTimer       = 0.0f;
    waitingForRelease = releaseTimer > 0.0f;
    exitingHouse      = releaseTimer <= 0.0f;
}

// The ghost bounces up and down inside the house while waiting to be released.
void Ghost::UpdateWaitingInsideHouse(float deltaTime)
{
    releaseTimer -= deltaTime;
    bounceTimer  += deltaTime * BOUNCE_SPEED;
    position.x    = spawnPosition.x;
    position.y    = spawnPosition.y + std::sin(bounceTimer) * BOUNCE_DISTANCE;

    if (releaseTimer <= 0.0f)
        StartLeavingHouse();
}

void Ghost::StartLeavingHouse()
{
    waitingForRelease = false;
    exitingHouse      = true;
    position          = spawnPosition;
    currentDirection  = MakeVector2D(0.0f, -1.0f);
}

// Works out the next waypoint the ghost should reach on its way out of the house.
Vector2D Ghost::GetCurrentExitTarget() const
{
    Vector2D door    = manager->TileToWorldCenter(GetDoorColumn(), DOOR_ROW);
    Vector2D hallway = manager->TileToWorldCenter(GetMazeExitColumn(), HALLWAY_ROW);
    Vector2D exit    = manager->TileToWorldCenter(GetMazeExitColumn(), MAZE_EXIT_ROW);

    // Step 1: slide horizontally to line up with the door.
    if (std::fabs(position.x - door.x) > ALIGN_TOLERANCE && position.y > hallway.y + ALIGN_TOLERANCE)
        return MakeVector2D(door.x, position.y);

    // Step 2: move up through the door.
    if (position.y > hallway.y + ALIGN_TOLERANCE)
        return MakeVector2D(door.x, hallway.y);

    // Step 3: slide to the exit column.
    if (std::fabs(position.x - hallway.x) > ALIGN_TOLERANCE)
        return hallway;

    // Step 4: move up into the maze.
    return exit;
}

// Moves the ghost toward a target point without overshooting it.
void Ghost::MoveTowardPoint(Vector2D target, float distance)
{
    if (std::fabs(position.x - target.x) > ALIGN_TOLERANCE)
    {
        float dir  = (position.x < target.x) ? 1.0f : -1.0f;
        float step = std::min(distance, std::fabs(position.x - target.x));
        currentDirection = MakeVector2D(dir, 0.0f);
        position.x += dir * step;
        return;
    }

    if (std::fabs(position.y - target.y) > ALIGN_TOLERANCE)
    {
        float dir  = (position.y < target.y) ? 1.0f : -1.0f;
        float step = std::min(distance, std::fabs(position.y - target.y));
        currentDirection = MakeVector2D(0.0f, dir);
        position.y += dir * step;
        return;
    }

    position = target;
}

void Ghost::UpdateExitFromHouse(float distance)
{
    Vector2D target = GetCurrentExitTarget();
    MoveTowardPoint(target, distance);

    Vector2D mazeExit = manager->TileToWorldCenter(GetMazeExitColumn(), MAZE_EXIT_ROW);
    if (position.y <= mazeExit.y + ALIGN_TOLERANCE)
    {
        exitingHouse = false;
        position     = mazeExit;
        SetChase();
    }
}

// Normal maze movement: align to the lane, pick a direction at intersections, then move.
void Ghost::MoveThroughMaze(float distance)
{
    manager->AlignToTileCenter(position, currentDirection);

    if (manager->IsNearTileCenter(position, 1.0f))
    {
        // Pick the best direction at this intersection.
        Vector2D target    = GetChaseTarget();
        Vector2D nextDir   = manager->ChooseGhostDirection(this, target, IsFrightened(), allowReverse);
        if (!IsZeroVector(nextDir))
            currentDirection = nextDir;
        allowReverse = false;
    }

    bool canUseDoor = (state == GHOST_STATE_EATEN);
    bool atCenter   = manager->IsNearTileCenter(position, 1.0f);

    bool blocked = atCenter && !manager->CanEnterNeighborTile(position, currentDirection, canUseDoor, canUseDoor);
    if (!blocked && manager->CanMove(position, currentDirection, distance, canUseDoor, canUseDoor))
    {
        position.x += currentDirection.x * distance;
        position.y += currentDirection.y * distance;
        manager->WrapPosition(position);
    }
}

// Checks if an eaten ghost has reached its home tile and should start leaving again.
void Ghost::FinishEatenTripIfNeeded()
{
    if (state != GHOST_STATE_EATEN)
        return;

    Vector2D currentTile = manager->WorldToTile(position);
    Vector2D homeTile    = manager->WorldToTile(manager->GetGhostHouseTarget());
    if (AreVectorsEqual(currentTile, homeTile))
    {
        position     = manager->GetGhostHouseTarget();
        SetChase();
        exitingHouse     = true;
        currentDirection = MakeVector2D(0.0f, -1.0f);
    }
}

void Ghost::Render()
{
    // Eaten ghosts are invisible; they are just moving home.
    if (state == GHOST_STATE_EATEN)
        return;

    Texture2D* tex = IsFrightened() ? frightenedTexture : chaseTexture;
    float size     = (float)GameManager::TileSize;

    Rectangle source = { 0, 0, 16, 16 };
    Rectangle dest   = { position.x, position.y, size, size };
    Vector2 origin   = { size * 0.5f, size * 0.5f };
    DrawTexturePro(*tex, source, dest, origin, 0.0f, WHITE);
}

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
