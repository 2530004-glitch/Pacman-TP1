#pragma once

/*
    entrypoint.h
    ------------
    This file exposes the small C-style function that starts the game.
    The platform-specific startup code in main.cpp calls this function.
*/

#ifdef __cplusplus
extern "C" {
#endif

void game_start(void);

#ifdef __cplusplus
}
#endif
