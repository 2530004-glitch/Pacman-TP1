#pragma once

#include "GameManager.h"
#include "raylib.h"

// This is the main Game class. Owns the window loop, menus, textures, and state transitions.
// All actual gameplay happens inside the GameManager, Game just decides which screen to show.
class Game
{
public:
    enum State { MainMenu, Playing, GameOver, Victory, Rules };

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
    State state;
    int   menuSelection;
    bool  assetsLoaded;

    void LoadAssets();
    void UnloadAssets();

    void DrawMenu() const;
    void DrawRulesScreen() const;
    void DrawEndScreen(const char* title, const char* subtitle, Color color) const;

    void UpdateMenu();
    void UpdateRules();
    void UpdatePlaying();
    void UpdateEnding();

public:
    Game();
    ~Game();

    void start();
    void update();
    void render();
    void end();

    float GetDeltaTime() const;
    State GetState() const;
    void  SetState(State next);
};
