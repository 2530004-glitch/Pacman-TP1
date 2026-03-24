/*
    Pellet.cpp
    ----------
    This file implements the Pellet class.
    Pellets do not move, but they do draw themselves with a different size
    depending on whether they are normal pellets or super pellets.
*/

#include "Pellet.h"

#include "GameManager.h"

// Creates a new pellet object.
// owner is the GameManager, startPosition is the world position,
// isSuper decides the pellet type, and spriteTexture is the draw texture.
Pellet::Pellet(GameManager* owner, Vector2D startPosition, bool isSuper, Texture2D* spriteTexture)
    : GameObject(TAG_PELLET)
{
    manager = owner;
    texture = spriteTexture;
    superPellet = isSuper;
    position = startPosition;
}

// Returns the size used when drawing the pellet.
float Pellet::GetDrawSize() const
{
    if (superPellet)
    {
        return (float)manager->GetTileSize();
    }

    return (float)manager->GetTileSize() * 0.6f;
}

// Returns true if this pellet is a super pellet.
bool Pellet::IsSuper() const
{
    return superPellet;
}

// Draws the pellet texture at its position.
void Pellet::Render()
{
    Rectangle source;
    source.x = 0.0f;
    source.y = 0.0f;
    source.width = 16.0f;
    source.height = 16.0f;

    float size = GetDrawSize();
    Rectangle destination;
    destination.x = position.x;
    destination.y = position.y;
    destination.width = size;
    destination.height = size;

    Vector2 origin;
    origin.x = size * 0.5f;
    origin.y = size * 0.5f;
    DrawTexturePro(*texture, source, destination, origin, 0.0f, WHITE);
}

// Updates the pellet.
void Pellet::Update()
{
    // Pellets do not move or animate, so no update work is needed here.
}
