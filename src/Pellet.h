#pragma once

#include "GameObject.h"
#include "raylib.h"

class GameManager;

class Pellet : public GameObject
{
private:
    GameManager* manager;
    Texture2D* texture;
    bool superPellet;

public:
    Pellet(GameManager* owner, Vector2D startPosition, bool isSuper, Texture2D* spriteTexture);

    bool IsSuper() const;
    void Render();
    void Update();
};
