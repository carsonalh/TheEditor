#pragma once

namespace ed
{

struct Mat2
{
    float a00, a01;
    float a10, a11;

public:
    inline Mat2 operator*(Mat2 other) const
    {
        return Mat2 {
            a00 * other.a00 + a01 * other.a10, a00 * other.a00 + a01 * other.a11,
            a10 * other.a00 + a11 * other.a10, a10 * other.a00 + a11 * other.a11,
        };
    }

    inline float det() const
    {
        return a00 * a11 - a01 * a10;
    }

    inline Mat2 inverse() const
    {
        float invdet = 1.0f / det();
        return Mat2 {
            a11 * invdet, -a01 * invdet,
            -a10 * invdet, a00 * invdet,
        };
    }
};

struct Vec2
{
    float x, y;

public:
    inline Vec2 operator+(Vec2 other) const
    {
        return { x + other.x, y + other.y };
    }

    inline Vec2 operator*(float scalar) const
    {
        return { scalar * x, scalar * y };
    }

    inline Vec2 operator-(Vec2 other) const
    {
        return *this + other * -1.0f;
    }
};

struct Rect
{
    float x, y;
    float width, height;

public:
    inline bool contains(Vec2 point)
    {
        const float left = x, right = x + width;
        const float top = y, bottom = y + height;

        return left <= point.x && point.x <= right &&
                top <= point.y && point.y <= bottom;
    }

    inline Rect add_offset(Vec2 offset)
    {
        return {
            x + offset.x, y + offset.y,
            width, height,
        };
    }
};

inline Vec2 operator*(float scalar, Vec2 v)
{
    return v * scalar;
}

inline Vec2 operator*(Mat2 A, Vec2 v)
{
    return {
        A.a00 * v.x + A.a01 * v.y,
        A.a10 * v.x + A.a11 * v.y,
    };
}

}
