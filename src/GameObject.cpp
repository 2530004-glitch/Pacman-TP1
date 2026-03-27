#include "GameObject.h"

GameObject::GameObject(ObjectTag objectTag)
{
    active = true;
    position = MakeVector2D(0.0f, 0.0f);
    tag = objectTag;
}

GameObject::~GameObject()
{
}

Vector2D GameObject::GetPosition() const
{
    return position;
}

void GameObject::SetPosition(Vector2D newPosition)
{
    position = newPosition;
}

bool GameObject::IsActive() const
{
    return active;
}

void GameObject::SetActive(bool isActive)
{
    active = isActive;
}

ObjectTag GameObject::GetTag() const
{
    return tag;
}
