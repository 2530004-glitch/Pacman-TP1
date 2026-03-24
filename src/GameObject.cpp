/*
    GameObject.cpp
    --------------
    This file implements the shared GameObject base class.
    The derived classes use these simple getters and setters to store
    position, active state, and object type.
*/

#include "GameObject.h"

// Creates a new game object with a starting tag and default values.
// objectTag says what kind of object this is.
GameObject::GameObject(ObjectTag objectTag)
{
    // New objects begin active so they can update and draw.
    active = true;

    // New objects begin at position 0,0 until another class moves them.
    position = MakeVector2D(0.0f, 0.0f);

    // Store the type so the manager can recognize this object later.
    tag = objectTag;
}

// Virtual destructor for safe cleanup through a base class pointer.
GameObject::~GameObject()
{
}

// Returns the current world position of the object.
Vector2D GameObject::GetPosition() const
{
    // Give back the stored position.
    return position;
}

// Changes the world position of the object.
// newPosition is the new center point in the game world.
void GameObject::SetPosition(Vector2D newPosition)
{
    // Store the new position directly.
    position = newPosition;
}

// Returns true if the object should still update and draw.
bool GameObject::IsActive() const
{
    // Give back the stored active flag.
    return active;
}

// Turns the object on or off.
// isActive decides if the object stays in play.
void GameObject::SetActive(bool isActive)
{
    // Store the new active state.
    active = isActive;
}

// Returns the tag that identifies the type of object.
ObjectTag GameObject::GetTag() const
{
    // Give back the stored object type.
    return tag;
}
