#pragma once

#include "GameManager.h"
#include "raylib.h"

/*
    Game.h
    ------
    This file defines the main Game class.
    The Game class owns the window loop, menu screens, game state, and all
    textures. It also owns the GameManager, which handles the actual Pac-Man
    gameplay inside the maze.
*/

class Game
{
public:
    enum StateType
    {
        MainMenuState,
        PlayingState,
        GameOverState,
        VictoryState,
        RulesState
    };

private:
    GameManager manager;
    Texture2D pacManTexture;
    Texture2D coinTexture;
    Texture2D bigCoinTexture;
    Texture2D inkyTexture;
    Texture2D pinkyTexture;
    Texture2D clydeTexture;
    Texture2D frightenedTexture;
    Texture2D wallTexture;
    float deltaTime;
    float stateTimer;
    StateType state;
    int menuSelection;
    bool assetsLoaded;

    void LoadRequiredTexture(Texture2D* texture, const char* path);
    void LoadCherryTexture(Texture2D* texture);
    void LoadAssets();
    void UnloadAssets();
    void UnloadTextureIfLoaded(Texture2D* texture);

    void DrawMenu() const;
    void DrawMenuTitle() const;
    void DrawMenuButtons() const;
    void DrawMenuHints() const;
    void DrawRulesScreen() const;
    void DrawRulesControls() const;
    void DrawRulesObjective() const;
    void DrawRulesScoring() const;
    void DrawEndScreen(const char* title, const char* subtitle, Color color) const;

    void UpdateMainMenu();
    void UpdateRulesState();
    void UpdatePlayingState();
    void UpdateEndingState();

public:
    Game();
    ~Game();

    void start();
    void update();
    void render();
    void end();

    float GetDeltaTime() const;
    StateType GetState() const;
    void SetState(StateType nextState);
};
