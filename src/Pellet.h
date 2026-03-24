#pragma once

#include "GameObject.h"
#include "raylib.h"

/*
    Pellet.h
    --------
    This file defines the Pellet class.
    Pellets inherit from GameObject and represent both normal pellets and
    the larger cherry-powered pellets placed around the map.
*/

class GameManager;

class Pellet : public GameObject
{
private:
    GameManager* manager;
    Texture2D* texture;
    bool superPellet;

    float GetDrawSize() const;

public:
    Pellet(GameManager* owner, Vector2D startPosition, bool isSuper, Texture2D* spriteTexture);

    bool IsSuper() const;
    void Render();
    void Update();
};
