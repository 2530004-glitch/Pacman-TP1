#pragma once

/*
    Vector2D.h
    ----------
    This file defines the very simple Vector2D struct used for positions
    and directions in the Pac-Man game.
    The struct only stores x and y. Small helper functions are provided
    so the rest of the game can do vector math in a clear way.
*/

struct Vector2D
{
    float x;
    float y;
};

// Creates a Vector2D with the given x and y values.
inline Vector2D MakeVector2D(float xValue, float yValue)
{
    // Put the numbers into a simple struct and return it.
    Vector2D result;
    result.x = xValue;
    result.y = yValue;
    return result;
}

// Adds two vectors together and returns the result.
inline Vector2D AddVectors(Vector2D first, Vector2D second)
{
    // Add each matching axis.
    Vector2D result;
    result.x = first.x + second.x;
    result.y = first.y + second.y;
    return result;
}

// Multiplies a vector by a number and returns the result.
inline Vector2D MultiplyVector(Vector2D value, float scalar)
{
    // Multiply both axes by the same amount.
    Vector2D result;
    result.x = value.x * scalar;
    result.y = value.y * scalar;
    return result;
}

// Returns true when both vectors store the same x and y values.
inline bool AreVectorsEqual(Vector2D first, Vector2D second)
{
    // Compare each axis directly.
    if (first.x != second.x)
    {
        return false;
    }

    if (first.y != second.y)
    {
        return false;
    }

    return true;
}

// Returns true when the vector has no movement on either axis.
inline bool IsZeroVector(Vector2D value)
{
    // A zero vector means x and y are both zero.
    Vector2D zeroVector = MakeVector2D(0.0f, 0.0f);
    return AreVectorsEqual(value, zeroVector);
}
