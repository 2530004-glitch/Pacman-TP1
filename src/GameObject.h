#pragma once

#include "ObjectTag.h"
#include "Vector2D.h"

/*
    GameObject.h
    ------------
    This file defines the base GameObject class.
    Every visible or updatable thing in the game inherits from this class,
    including Pac-Man, ghosts, and pellets.
*/

class GameObject
{
protected:
    bool active;
    Vector2D position;
    ObjectTag tag;

public:
    GameObject(ObjectTag objectTag);
    virtual ~GameObject();

    Vector2D GetPosition() const;
    void SetPosition(Vector2D newPosition);
    bool IsActive() const;
    void SetActive(bool isActive);
    ObjectTag GetTag() const;

    virtual void Render() = 0;
    virtual void Update() = 0;
};
