#pragma once

struct Vector2D
{
    float x;
    float y;
};

inline Vector2D MakeVector2D(float x, float y)
{
    Vector2D v;
    v.x = x;
    v.y = y;
    return v;
}

inline Vector2D AddVectors(Vector2D a, Vector2D b)
{
    return MakeVector2D(a.x + b.x, a.y + b.y);
}

inline Vector2D MultiplyVector(Vector2D v, float scalar)
{
    return MakeVector2D(v.x * scalar, v.y * scalar);
}

inline bool AreVectorsEqual(Vector2D a, Vector2D b)
{
    return a.x == b.x && a.y == b.y;
}

inline bool IsZeroVector(Vector2D v)
{
    return v.x == 0.0f && v.y == 0.0f;
}
