#ifndef LINMATH_H
#define LINMATH_H

typedef struct { float x, y; } Vec2;
typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y, z, w; } Vec4;

static inline Vec2 v2_add(Vec2 a, Vec2 b)
{
    return (Vec2) { a.x + b.x, a.y + b.y };
}

static inline float v2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

static inline Vec2 v2_scale(float a, Vec2 x)
{
    return (Vec2) { a * x.x, a * x.y };
}

static inline Vec3 v3_add(Vec3 a, Vec3 b)
{
    return (Vec3) { a.x + b.x, a.y + b.y, a.z + b.z };
}

static inline float v3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 v3_scale(float a, Vec3 x)
{
    return (Vec3) { a * x.x, a * x.y, a * x.z };
}

static inline Vec3 v3_cross(Vec3 a, Vec3 b)
{
    return (Vec3) {
        a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x,
    };
}

static inline Vec4 v4_add(Vec4 a, Vec4 b)
{
    return (Vec4) {
        a.x + b.x,
		a.y + b.y,
		a.z + b.z,
		a.w + b.w,
    };
}

static inline float v4_dot(Vec4 a, Vec4 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

static inline Vec4 v4_scale(float a, Vec4 x)
{
    return (Vec4) {a * x.x, a * x.y, a * x.z, a * x.w};
}

#endif // LINMATH_H
