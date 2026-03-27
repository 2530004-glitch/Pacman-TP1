#include "Pellet.h"
#include "GameManager.h"

Pellet::Pellet(GameManager* owner, Vector2D startPosition, bool isSuper, Texture2D* spriteTexture)
    : GameObject(TAG_PELLET)
{
    manager = owner;
    texture = spriteTexture;
    superPellet = isSuper;
    position = startPosition;
}

bool Pellet::IsSuper() const
{
    return superPellet;
}

void Pellet::Render()
{
    float size = superPellet
        ? (float)GameManager::TileSize
        : (float)GameManager::TileSize * 0.6f;

    Rectangle source = { 0, 0, 16, 16 };
    Rectangle dest   = { position.x, position.y, size, size };
    Vector2 origin   = { size * 0.5f, size * 0.5f };

    DrawTexturePro(*texture, source, dest, origin, 0.0f, WHITE);
}

void Pellet::Update()
{
    //maybe will add a blinking effect later for pellets
}