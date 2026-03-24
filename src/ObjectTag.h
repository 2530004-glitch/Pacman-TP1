#pragma once

/*
    ObjectTag.h
    -----------
    This file stores the plain enums used across the whole Pac-Man game.
    These enums help the rest of the code identify objects, ghost types,
    and ghost behavior states without using hard to read numbers.
*/

enum ObjectTag
{
    TAG_PACMAN,   // Used to identify the Pac-Man object
    TAG_GHOST,    // Used to identify any ghost object
    TAG_PELLET    // Used to identify any pellet object
};

enum GhostType
{
    GHOST_INKY,   // Green ghost that patrols its quadrant until Pac-Man enters it
    GHOST_PINKY,  // Yellow ghost that targets tiles in front of Pac-Man
    GHOST_CLYDE   // Orange ghost that chases or retreats based on distance
};

enum GhostState
{
    GHOST_STATE_CHASE,       // Normal ghost behavior
    GHOST_STATE_FRIGHTENED,  // Blue ghost behavior after Pac-Man eats a cherry
    GHOST_STATE_EATEN        // Ghost returning to the ghost house after being eaten
};
