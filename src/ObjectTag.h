#pragma once

// Tags that identify what kind of object something is.
enum ObjectTag
{
    TAG_PACMAN,
    TAG_GHOST,
    TAG_PELLET
};

// Which ghost this is. Each ghost has different AI behavior.
enum GhostType
{
    GHOST_INKY,   // Patrols the top-right quadrant; chases Pac-Man when he enters it
    GHOST_PINKY,  // Targets a few tiles ahead of Pac-Man
    GHOST_CLYDE   // Chases when far away, retreats when close
};

// What a ghost is currently doing.
enum GhostState
{
    GHOST_STATE_CHASE,      // Normal chasing behavior
    GHOST_STATE_FRIGHTENED, // Turned blue after Pac-Man eats a cherry
    GHOST_STATE_EATEN       // Returning at spawn position after being eaten
};
