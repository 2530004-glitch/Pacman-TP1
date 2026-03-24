/*
    entrypoint.cpp
    --------------
    This file creates the main Game object and starts the game loop.
    It is kept very small so the program startup is easy to follow.
*/

#include "Game.h"
#include "entrypoint.h"

// Starts the Pac-Man game.
void game_start(void)
{
    // Create the main Game object on the stack.
    Game game;

    // Run the main game loop.
    game.start();
}
