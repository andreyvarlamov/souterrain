#ifndef VA_LINMATH_H
#define VA_LINMATH_H

#include "va_types.h"
#include "va_math.h"
#include "va_colors.h"

// NOTE: All mat functions assume Column-Major representation [Column][Row]

// -------------------------------------------------------------------------------
// SECTION: VECTOR 2 -------------------------------------------------------------
// -------------------------------------------------------------------------------

union vec2
{
    struct
    {
        f32 X, Y;
    };

    f32 E[2];
};

inline vec2 Vec2() { return { 0.0f, 0.0f }; }
inline vec2 Vec2(f32 X, f32 Y) { return { X, Y }; }
inline vec2 Vec2(int X, int Y) { return { (f32) X, (f32) Y }; }
inline vec2 Vec2(f32 V) { return { V, V }; }

inline vec2 operator+(vec2 V0, vec2 V1) { return { V0.X + V1.X, V0.Y + V1.Y }; }
inline vec2 operator-(vec2 V0, vec2 V1) { return { V0.X - V1.X, V0.Y - V1.Y }; }
inline vec2 operator-(vec2 V0) { return { -V0.X, -V0.Y }; }
inline vec2 operator*(vec2 V, f32 S) { return { V.X * S, V.Y * S }; }
inline vec2 operator*(f32 S, vec2 V) { return { V.X * S, V.Y * S }; }
inline vec2 operator*(vec2 V0, vec2 V1) { return { V0.X * V1.X, V0.Y * V1.Y }; }
inline vec2 operator/(vec2 V, f32 S) { return { V.X / S, V.Y / S }; }
inline vec2 &operator+=(vec2 &V0, vec2 V1) { V0 = V0 + V1; return V0; }
inline vec2 &operator-=(vec2 &V0, vec2 V1) { V0 = V0 - V1; return V0; }
inline vec2 &operator*=(vec2 &V, f32 S) { V = V * S; return V; }
inline vec2 &operator/=(vec2 &V, f32 S) { V = V / S; return V; }
inline f32 VecDot(vec2 V0, vec2 V1) { return V0.X * V1.X + V0.Y * V1.Y; }
inline f32 VecLengthSq(vec2 V) { return V.X * V.X + V.Y * V.Y; }
inline f32 VecLength(vec2 V) { return SqrtF(V.X * V.X + V.Y * V.Y); }
inline vec2 VecNormalize(vec2 V) { f32 Length = VecLength(V); Assert(Length > 0.0f); return { V.X / Length, V.Y / Length }; }

// TODO: VecClamp is inefficient (still want to be fast ish in debug)
inline vec2
VecClamp(vec2 V, vec2 Min, vec2 Max)
{
    if (V.X < Min.X) V.X = Min.X; else if (V.X > Max.X) V.X = Max.X;
    if (V.Y < Min.Y) V.Y = Min.Y; else if (V.Y > Max.Y) V.Y = Max.Y;
    return V;
}
inline vec2 VecClamp(vec2 V, f32 Min, f32 Max) { return VecClamp(V, Vec2(Min), Vec2(Max)); }
inline vec2 VecClamp(vec2 V, vec2 Abs) { return VecClamp(V, -Abs, Abs); }
inline vec2 VecClamp(vec2 V, f32 Abs) { return VecClamp(V, -Abs, Abs); }

// -------------------------------------------------------------------------------
// SECTION: VECTOR 3 -------------------------------------------------------------
// -------------------------------------------------------------------------------

union vec3
{
    struct
    {
        f32 X, Y, Z;
    };

    struct
    {
        f32 R, G, B;
    };

    f32 E[3];
};

inline vec3 Vec3() { return { 0.0f, 0.0f, 0.0f }; }
inline vec3 Vec3(f32 X, f32 Y, f32 Z) { return { X, Y, Z}; }
inline vec3 Vec3(f32 Value) { return { Value, Value, Value }; }
inline vec3 Vec3(vec2 V, f32 Z) { return { V.X, V.Y, Z }; }
inline vec3 Vec3(vec2 V) { return { V.X, V.Y, 0.0f }; }
inline vec2 Vec2(vec3 V) { return { V.X, V.Y }; }
inline vec3 operator+(vec3 V0, vec3 V1) { return { V0.X + V1.X, V0.Y + V1.Y, V0.Z + V1.Z }; }
inline vec3 operator-(vec3 V0, vec3 V1) { return { V0.X - V1.X, V0.Y - V1.Y, V0.Z - V1.Z }; }
inline vec3 operator-(vec3 V0) { return { -V0.X, -V0.Y, -V0.Z }; }
inline vec3 operator*(vec3 V0, vec3 V1) { return { V0.X * V1.X, V0.Y * V1.Y, V0.Z * V1.Z }; }
inline vec3 operator*(vec3 V, f32 S) { return { V.X * S, V.Y * S, V.Z * S }; }
inline vec3 operator*(f32 S, vec3 V) { return { V.X * S, V.Y * S, V.Z * S }; }
inline vec3 operator/(vec3 V, f32 S) { return { V.X / S, V.Y / S, V.Z / S }; }
inline vec3 operator/(f32 S, vec3 V) { return { S / V.X, S / V.Y, S / V.Z }; }
inline vec3 &operator+=(vec3 &V0, vec3 V1) { V0 = V0 + V1; return V0; }
inline vec3 &operator-=(vec3 &V0, vec3 V1) { V0 = V0 - V1; return V0; }
inline vec3 &operator*=(vec3 &V, f32 S) { V = V * S; return V; }
inline vec3 &operator/=(vec3 &V, f32 S) { V = V / S; return V; }

inline f32 VecDot(vec3 V0, vec3 V1) { return V0.X * V1.X + V0.Y * V1.Y + V0.Z * V1.Z; }
inline f32 VecLengthSq(vec3 V) { return V.X * V.X + V.Y * V.Y + V.Z * V.Z; }
inline f32 VecLength(vec3 V) { return SqrtF(V.X * V.X + V.Y * V.Y + V.Z * V.Z); }
inline vec3 VecNormalize(vec3 V) { f32 Length = SqrtF(V.X * V.X + V.Y * V.Y + V.Z * V.Z); Assert(Length > 0.0f); return { V.X / Length, V.Y / Length, V.Z / Length }; } 
inline vec3 VecCross(vec3 V0, vec3 V1) { return { V0.Y * V1.Z - V0.Z * V1.Y, V0.Z * V1.X - V0.X * V1.Z, V0.X * V1.Y - V0.Y * V1.X }; }
inline f32 VecScalarTriple(vec3 A, vec3 B, vec3 C) { return VecDot(A, VecCross(B, C)); }
inline b32 IsZeroVector(vec3 Vector) { return ((AbsF(Vector.X) <= FLT_EPSILON) && (AbsF(Vector.Y) <= FLT_EPSILON) && (AbsF(Vector.Z) <= FLT_EPSILON)); }

inline b32
AreVecEqual(vec3 A, vec3 B)
{
    b32 Result = IsZeroVector(A - B);
    return Result;
}

inline vec3
Vec3Lerp(vec3 A, vec3 B, f32 LerpFactor)
{
    vec3 Result = A + LerpFactor * (B - A);
    return Result;
}

// -------------------------------------------------------------------------------
// SECTION: RECTANGLE ------------------------------------------------------------
// -------------------------------------------------------------------------------

struct rect
{
    f32 X;
    f32 Y;
    f32 Width;
    f32 Height;
};

inline rect Rect(f32 X, f32 Y, f32 Width, f32 Height) { return { X, Y, Width, Height }; }
inline rect Rect(f32 Width, f32 Height) { return { 0, 0, Width, Height }; }
inline rect Rect(f32 Dim) { return { 0, 0, Dim, Dim }; }
inline rect RectScale(rect R, f32 Scale) { R.Width *= Scale; R.Height *= Scale; return R; }
inline rect Rect(i32 X, i32 Y, i32 Width, i32 Height) { return { (f32) X, (f32) Y, (f32) Width, (f32) Height }; }
inline rect Rect(i32 Width, i32 Height) { return { 0.0f, 0.0f, (f32) Width, (f32) Height }; }
inline rect Rect(i32 Dim) { return { 0.0f, 0.0f, (f32) Dim, (f32) Dim }; }
inline rect Rect(vec2 Dim) { return { 0.0f, 0.0f, Dim.X, Dim.Y }; }
inline rect Rect(vec2 Min, vec2 Dim) { return { Min.X, Min.Y, Dim.X, Dim.Y }; }
inline rect Rect(f32 X, f32 Y, vec2 Dim) { return { X, Y, Dim.X, Dim.Y }; }
inline rect RectMinMax(vec2 Min, vec2 Max) { return { Min.X, Min.Y, Max.X - Min.X, Max.Y - Min.Y }; }
inline vec2 RectGetMin(rect Rect) { return { Rect.X, Rect.Y }; }
inline vec2 RectGetMax(rect Rect) { return { Rect.X + Rect.Width, Rect.Y + Rect.Height }; }
inline vec2 RectGetMid(rect Rect) { return { Rect.X + Rect.Width / 2.0f, Rect.Y + Rect.Height / 2.0f }; }

inline void
RectGetPoints(rect Rect, vec2 *Points)
{
    Points[0] = { Rect.X, Rect.Y };
    Points[1] = { Rect.X, Rect.Y + Rect.Height };
    Points[2] = { Rect.X + Rect.Width, Rect.Y + Rect.Height };
    Points[3] = { Rect.X + Rect.Width, Rect.Y };
}

inline void
RectGetPoints(rect Rect, vec3 *Points)
{
    Points[0] = { Rect.X, Rect.Y, 0.0f };
    Points[1] = { Rect.X, Rect.Y + Rect.Height, 0.0f };
    Points[2] = { Rect.X + Rect.Width, Rect.Y + Rect.Height, 0.0f };
    Points[3] = { Rect.X + Rect.Width, Rect.Y, 0.0f };
}

// -------------------------------------------------------------------------------
// SECTION: VECTOR 4 -------------------------------------------------------------
// -------------------------------------------------------------------------------

union vec4
{
    struct
    {
        f32 X, Y, Z, W;
    };

    struct
    {
        f32 R, G, B, A;
    };

    f32 E[4];
};

inline vec4 Vec4() { vec4 Result = {}; return Result; }
inline vec4 Vec4(f32 X, f32 Y, f32 Z, f32 W) { vec4 Result = {}; Result.X = X; Result.Y = Y; Result.Z = Z; Result.W = W; return Result; }
inline vec4 Vec4(vec3 V, f32 W) { return Vec4(V.X, V.Y, V.Z, W); }
inline vec4 Vec4(vec3 V) { return Vec4(V, 0.0f); }
inline vec4 Vec4(vec2 V, f32 Z, f32 W) { return Vec4(V.X, V.Y, Z, W); }
inline vec4 Vec4(vec2 V1, vec2 V2) { return Vec4(V1.X, V1.Y, V2.X, V2.Y); }
inline vec3 Vec3(vec4 V) { return Vec3(V.X, V.Y, V.Z);}
inline vec2 Vec2(vec4 V) { return Vec2(V.X, V.Y); }
inline vec4 operator+(vec4 V0, vec4 V1) { return Vec4(V0.X + V1.X, V0.Y + V1.Y, V0.Z + V1.Z, V0.W + V1.W); }
inline vec4 operator-(vec4 V0) { return Vec4(-V0.X, -V0.Y, -V0.Z, -V0.W); }
inline vec4 operator-(vec4 V0, vec4 V1) { return V0 + (-V1); }
inline vec4 operator*(vec4 V, f32 S) { return Vec4(V.X * S, V.Y * S, V.Z * S, V.W * S); }
inline vec4 operator*(f32 S, vec4 V) { return V * S; }
inline vec4 operator/(vec4 V, f32 S);
inline vec4 &operator+=(vec4 &V0, vec4 V1);
inline vec4 &operator-=(vec4 &V0, vec4 V1);
inline vec4 &operator*=(vec4 &V, f32 S);
inline vec4 &operator/=(vec4 &V, f32 S);

inline vec4
Vec4Lerp(vec4 A, vec4 B, f32 LerpFactor)
{
    return A + LerpFactor * (B - A);
}

// NOTE: V4-Color helpers
inline color Color(vec4 ColorV) { color Color; Color.R = (u8) (ColorV.R * 255); Color.G = (u8) (ColorV.G * 255); Color.B = (u8) (ColorV.B * 255); Color.A = (u8) (ColorV.A * 255); return Color; }
inline vec4 ColorV4(color Color) { vec4 ColorV; ColorV.R = Color.R / 255.0f; ColorV.G = Color.G / 255.0f; ColorV.B = Color.B / 255.0f; ColorV.A = Color.A / 255.0f; return ColorV; }
inline vec4 ColorV4(u8 R, u8 G, u8 B, u8 A) { return ColorV4(Color(R, G, B, A)); }
inline vec4 ColorV4(u8 R, u8 G, u8 B) { return ColorV4(Color(R, G, B)); }
inline vec4 ColorV4(u32 Color32) { return ColorV4(Color(Color32)); }

inline color
LerpColor(color A, color B, f32 T)
{
    vec4 VA = ColorV4(A);
    vec4 VB = ColorV4(B);
    
    vec4 VResult = Vec4Lerp(VA, VB, T);

    return Color(VResult);
}

// -------------------------------------------------------------------------------
// QUATERNION --------------------------------------------------------------------
// -------------------------------------------------------------------------------

union quat
{
    struct
    {
        f32 W, X, Y, Z;
    };

    f32 E[4];
};

inline quat
Quat()
{
    quat Result =  {};
    Result.W = 1.0f;
    return Result;
}

inline quat
Quat(f32 W, f32 X, f32 Y, f32 Z)
{
    quat Result =  {};

    Result.W = W;
    Result.X = X;
    Result.Y = Y;
    Result.Z = Z;
    
    return Result;
}

inline quat
Quat(vec3 Axis, f32 AngleRads)
{
    f32 HalfAngle = AngleRads * 0.5f;

    Axis = VecNormalize(Axis);
    
    quat Result = Quat(CosF(HalfAngle),
                       Axis.X * SinF(HalfAngle),
                       Axis.Y * SinF(HalfAngle),
                       Axis.Z * SinF(HalfAngle));

    return Result;
}

inline void
QuatGetAxisAngle(quat Q, f32 *Out_AngleRads, vec3 *Out_Axis)
{
    f32 HalfAngle = ArcCosF(Q.W);
    f32 SinHalfAngle = SinF(HalfAngle);
    vec3 Axis = {};
    if (AbsF(SinHalfAngle) > FLT_EPSILON)
    {
        Axis.X = Q.X / SinHalfAngle;
        Axis.Y = Q.Y / SinHalfAngle;
        Axis.Z = Q.Z / SinHalfAngle;
    }
    else
    {
        InvalidCodePath; // NOTE: Just want to check in which cases this happens
    }

    if (Out_AngleRads) *Out_AngleRads = HalfAngle * 2.0f;
    if (Out_Axis) *Out_Axis = Axis;
}

inline quat
operator-(quat Q)
{
    quat Result = quat { -Q.W, -Q.X, -Q.Y, -Q.Z };
    return Result;
}

inline quat
operator*(f32 Scalar, quat Q)
{
    quat Result = { Scalar * Q.W, Scalar * Q.X, Scalar * Q.Y, Scalar * Q.Z };
    return Result;
}

inline quat
operator*(quat Q, f32 Scalar)
{
    quat Result = (Scalar * Q);
    return Result;
}

inline quat
operator*(quat A, quat B)
{
    quat Result = {};
    Result.W = A.W * B.W - A.X * B.X - A.Y * B.Y - A.Z * B.Z;
    Result.X = A.W * B.X + A.X * B.W + A.Y * B.Z - A.Z * B.Y;
    Result.Y = A.W * B.Y + A.Y * B.W + A.Z * B.X - A.X * B.Z;
    Result.Z = A.W * B.Z + A.Z * B.W + A.X * B.Y - A.Y * B.X;
    return Result;
}

inline quat &
operator*=(quat &A, quat B)
{
    A = A * B;
    return A;
}

inline quat
operator/(quat Q, f32 Scalar)
{
    quat Result = { Q.W / Scalar, Q.X / Scalar, Q.Y / Scalar, Q.Z / Scalar };
    return Result;
}

inline quat
operator+(quat A, quat B)
{
    quat Result = quat { A.W + B.W, A.X + B.X, A.Y + B.Y, A.Z + B.Z };
    return Result;
}

inline quat
QuatConjugate(quat Q)
{
    quat Result = Quat(Q.W, -Q.X, -Q.Y, -Q.Z);
    return Result;
}

inline f32
QuatDot(quat A, quat B)
{
    f32 Result = A.W * B.W + A.X * B.X + A.Y * B.Y + A.Z * B.Z;
    return Result;
}

inline quat
QuatInverse(quat Q)
{
    quat Result = QuatConjugate(Q) / QuatDot(Q, Q);
    return Result;
}

inline quat
QuatSphericalLerp(quat A, quat B, f32 LerpFactor)
{
    // NOTE: Copy the homework from glm::slerp

    f32 CosTheta = QuatDot(A, B);

    // NOTE: If CosTheta < 0, the interpolation will take the long way around the sphere.
    // To fix this, negate one quat
    if (CosTheta < 0.0f)
    {
        B = -B;
        CosTheta -= CosTheta;
    }

    // NOTE: Perform a linear interpolation when CosTheta is close to 1 to avoid side effect
    // of sin(angle) becoming a zero denominator
    if (CosTheta >= (1.0f - FLT_EPSILON))
    {
        return quat {
            A.W + LerpFactor * (B.W - A.W),
            A.X + LerpFactor * (B.X - A.X),
            A.Y + LerpFactor * (B.Y - A.Y),
            A.Z + LerpFactor * (B.Z - A.Z)
        };
    }

    // NOTE: Essential Mathematics, p. 467
    f32 Angle = ArcCosF(CosTheta);
    quat Result = (SinF((1 - LerpFactor) * Angle) * A + SinF(LerpFactor * Angle) * B) / SinF(Angle);
    return Result;
}

inline vec3
RotateVecByQuatSlow(vec3 V, quat Q)
{
    quat P = Quat(0.0f, V.X, V.Y, V.Z);
    quat QConj = QuatConjugate(Q);

    quat QResult = Q * P * QConj;
    vec3 Result = Vec3(QResult.X, QResult.Y, QResult.Z);
    return Result;
}

// -------------------------------------------------------------------------------
// MATRIX 3 ----------------------------------------------------------------------
// -------------------------------------------------------------------------------

struct mat3
{
    f32 E[3][3];
};

inline mat3
Mat3()
{
    mat3 Result = {};
    return Result;
};

inline mat3
Mat3(f32 Diagonal)
{
    mat3 Result = {};

    Result.E[0][0] = Diagonal;
    Result.E[1][1] = Diagonal;
    Result.E[2][2] = Diagonal;

    return Result;
}

inline mat3
Mat3(vec3 A, vec3 B, vec3 C)
{
    mat3 Result = {};

    Result.E[0][0] = A.X;
    Result.E[0][1] = A.Y;
    Result.E[0][2] = A.Z;

    Result.E[1][0] = B.X;
    Result.E[1][1] = B.Y;
    Result.E[1][2] = B.Z;

    Result.E[2][0] = C.X;
    Result.E[2][1] = C.Y;
    Result.E[2][2] = C.Z;

    return Result;
}

inline mat3
Mat3(vec3 *Cols)
{
    Assert(Cols);

    mat3 Result = {};

    Result.E[0][0] = Cols[0].X;
    Result.E[0][1] = Cols[0].Y;
    Result.E[0][2] = Cols[0].Z;

    Result.E[1][0] = Cols[1].X;
    Result.E[1][1] = Cols[1].Y;
    Result.E[1][2] = Cols[1].Z;

    Result.E[2][0] = Cols[2].X;
    Result.E[2][1] = Cols[2].Y;
    Result.E[2][2] = Cols[2].Z;

    return Result;
}

inline mat3
operator*(mat3 M0, mat3 M1)
{
    mat3 Result = {};

    for (i32 I = 0; I < 3; ++I)
    {
        for (i32 J = 0; J < 3; ++J)
        {
            Result.E[J][I] = (M0.E[0][I]*M1.E[J][0] +
                              M0.E[1][I]*M1.E[J][1] +
                              M0.E[2][I]*M1.E[J][2]);
        }
    }

    return Result;
}

inline vec3
operator*(mat3 M, vec3 V)
{
    vec3 Result = {};

    Result.X = M.E[0][0] * V.X + M.E[1][0] * V.Y + M.E[2][0] * V.Z;
    Result.Y = M.E[0][1] * V.X + M.E[1][1] * V.Y + M.E[2][1] * V.Z;
    Result.Z = M.E[0][2] * V.X + M.E[1][2] * V.Y + M.E[2][2] * V.Z;

    return Result;
}

inline vec3
Mat3GetCol(mat3 M, u32 ColumnIndex)
{
    Assert(ColumnIndex < 3);

    vec3 Result;

    for (u32 RowIndex = 0; RowIndex < 3; ++RowIndex)
    {
        Result.E[RowIndex] = M.E[ColumnIndex][RowIndex];
    }

    return Result;
}

inline void
Mat3GetCols(mat3 M, vec3 *Out_Cols)
{
    Assert(Out_Cols);

    for (u32 ColIndex = 0; ColIndex < 3; ++ColIndex)
    {
        for (u32 RowIndex = 0; RowIndex < 3; ++RowIndex)
        {
            Out_Cols[ColIndex].E[RowIndex] = M.E[ColIndex][RowIndex];
        }
    }
}

inline mat3
Mat3Transpose(mat3 M)
{
    mat3 Result = {};

    for (u32 Column = 0; Column < 3; ++Column)
    {
        for (u32 Row = 0; Row < 3; ++Row)
        {
            Result.E[Column][Row] = M.E[Row][Column];
        }
    }

    return Result;
}

inline mat3
Mat3GetRotationAroundAxis(vec3 Axis, f32 Angle)
{
    f32 C = CosF(Angle);
    f32 S = SinF(Angle);
    
    vec3 Temp = (1.0f - C) * Axis;

    mat3 Result;

    Result.E[0][0] = C + Temp.E[0] * Axis.E[0];
    Result.E[0][1] = Temp.E[0] * Axis.E[1] + S * Axis.E[2];
    Result.E[0][2] = Temp.E[0] * Axis.E[2] - S * Axis.E[1];

    Result.E[1][0] = Temp.E[1] * Axis.E[0] - S * Axis.E[2];
    Result.E[1][1] = C + Temp.E[1] * Axis.E[1];
    Result.E[1][2] = Temp.E[1] * Axis.E[2] + S * Axis.E[0];

    Result.E[2][0] = Temp.E[2] * Axis.E[0] + S * Axis.E[1];
    Result.E[2][1] = Temp.E[2] * Axis.E[1] - S * Axis.E[0];
    Result.E[2][2] = C + Temp.E[2] * Axis.E[2];

    return Result;
}

// -------------------------------------------------------------------------------
// MATRIX 4 ----------------------------------------------------------------------
// -------------------------------------------------------------------------------

struct mat4
{
    f32 E[4][4];
};

inline mat4
Mat4()
{
    mat4 Result = {};
    return Result;
};

inline mat4
Mat4(f32 Diagonal)
{
    mat4 Result = {};

    Result.E[0][0] = Diagonal;
    Result.E[1][1] = Diagonal;
    Result.E[2][2] = Diagonal;
    Result.E[3][3] = Diagonal;

    return Result;
}

inline mat4
Mat4(vec4 A, vec4 B, vec4 C, vec4 D)
{
    mat4 Result = {};

    Result.E[0][0] = A.X;
    Result.E[0][1] = A.Y;
    Result.E[0][2] = A.Z;
    Result.E[0][3] = A.W;

    Result.E[1][0] = B.X;
    Result.E[1][1] = B.Y;
    Result.E[1][2] = B.Z;
    Result.E[1][3] = B.W;

    Result.E[2][0] = C.X;
    Result.E[2][1] = C.Y;
    Result.E[2][2] = C.Z;
    Result.E[2][3] = C.W;

    Result.E[3][0] = D.X;
    Result.E[3][1] = D.Y;
    Result.E[3][2] = D.Z;
    Result.E[3][3] = D.W;

    return Result;
}

inline mat4
Mat4(vec4 *Cols)
{
    Assert(Cols);

    mat4 Result = {};

    Result.E[0][0] = Cols[0].X;
    Result.E[0][1] = Cols[0].Y;
    Result.E[0][2] = Cols[0].Z;
    Result.E[0][3] = Cols[0].W;

    Result.E[1][0] = Cols[1].X;
    Result.E[1][1] = Cols[1].Y;
    Result.E[1][2] = Cols[1].Z;
    Result.E[1][3] = Cols[1].W;

    Result.E[2][0] = Cols[2].X;
    Result.E[2][1] = Cols[2].Y;
    Result.E[2][2] = Cols[2].Z;
    Result.E[2][3] = Cols[2].W;

    Result.E[3][0] = Cols[3].X;
    Result.E[3][1] = Cols[3].Y;
    Result.E[3][2] = Cols[3].Z;
    Result.E[3][3] = Cols[3].W;

    return Result;
}

inline mat4
Mat4(mat3 M)
{
    mat4 Result = Mat4(1.0f); // TODO: Is (3, 3) = 1 always when casting mat3 to mat4?

    Result.E[0][0] = M.E[0][0];
    Result.E[0][1] = M.E[0][1];
    Result.E[0][2] = M.E[0][2];
    
    Result.E[1][0] = M.E[1][0];
    Result.E[1][1] = M.E[1][1];
    Result.E[1][2] = M.E[1][2];

    Result.E[2][0] = M.E[2][0];
    Result.E[2][1] = M.E[2][1];
    Result.E[2][2] = M.E[2][2];

    return Result;
}

inline mat4
operator*(mat4 M0, mat4 M1)
{
    mat4 Result = {};

    for (i32 I = 0; I < 4; ++I)
    {
        for (i32 J = 0; J < 4; ++J)
        {
            Result.E[J][I] = (M0.E[0][I]*M1.E[J][0] +
                              M0.E[1][I]*M1.E[J][1] +
                              M0.E[2][I]*M1.E[J][2] +
                              M0.E[3][I]*M1.E[J][3]);
        }
    }

    return Result;
}

inline vec4
operator*(mat4 M, vec4 V)
{
    vec4 Result = {};

    Result.X = M.E[0][0] * V.X + M.E[1][0] * V.Y + M.E[2][0] * V.Z + M.E[3][0] * V.W;
    Result.Y = M.E[0][1] * V.X + M.E[1][1] * V.Y + M.E[2][1] * V.Z + M.E[3][1] * V.W;
    Result.Z = M.E[0][2] * V.X + M.E[1][2] * V.Y + M.E[2][2] * V.Z + M.E[3][2] * V.W;
    Result.W = M.E[0][2] * V.X + M.E[1][2] * V.Y + M.E[2][2] * V.Z + M.E[3][3] * V.W;

    return Result;
}

// ---------------------------------------------------------------------------------
// TRANSFORMS ----------------------------------------------------------------------
// ---------------------------------------------------------------------------------

inline mat4
Mat4GetTranslation(vec3 Translation)
{
    mat4 Result = Mat4(1.0f);
    
    Result.E[3][0] = Translation.X;
    Result.E[3][1] = Translation.Y;
    Result.E[3][2] = Translation.Z;

    return Result;
}

inline mat3
Mat3GetRotationFromQuat(quat Q)
{
    mat3 Result = Mat3(1.0f);

    f32 QXX = Q.X * Q.X;
    f32 QYY = Q.Y * Q.Y;
    f32 QZZ = Q.Z * Q.Z;
    f32 QXZ = Q.X * Q.Z;
    f32 QXY = Q.X * Q.Y;
    f32 QYZ = Q.Y * Q.Z;
    f32 QWX = Q.W * Q.X;
    f32 QWY = Q.W * Q.Y;
    f32 QWZ = Q.W * Q.Z;

    Result.E[0][0] = 1.0f - 2.0f * (QYY + QZZ);
    Result.E[0][1] = 2.0f * (QXY + QWZ);
    Result.E[0][2] = 2.0f * (QXZ - QWY);

    Result.E[1][0] = 2.0f * (QXY - QWZ);
    Result.E[1][1] = 1.0f - 2.0f * (QXX + QZZ);
    Result.E[1][2] = 2.0f * (QYZ + QWX);

    Result.E[2][0] = 2.0f * (QXZ + QWY);
    Result.E[2][1] = 2.0f * (QYZ - QWX);
    Result.E[2][2] = 1.0f - 2.0f * (QXX + QYY);
    
    return Result;
}

inline mat4
Mat4GetRotationFromQuat(quat Q)
{
    mat4 Result = Mat4(Mat3GetRotationFromQuat(Q));
    return Result;
}


inline mat3
Mat3GetScale(vec3 Scale)
{
    mat3 Result = Mat3();

    Result.E[0][0] = Scale.X;
    Result.E[1][1] = Scale.Y;
    Result.E[2][2] = Scale.Z;

    return Result;
}

inline mat4
Mat4GetScale(vec3 Scale)
{
    mat4 Result = Mat4(1.0f);

    Result.E[0][0] = Scale.X;
    Result.E[1][1] = Scale.Y;
    Result.E[2][2] = Scale.Z;

    return Result;
}

inline mat3
Mat3GetRotationAndScale(quat Rotation, vec3 Scale)
{
     // TODO: This can probably be optimized a lot
    mat3 RotationTransform = Mat3GetRotationFromQuat(Rotation);
    mat3 ScaleTransform = Mat3GetScale(Scale);
    
    mat3 Result = RotationTransform * ScaleTransform;
    return Result;
}

inline mat4
Mat4GetFullTransform(vec3 Position, quat Rotation, vec3 Scale)
{
     // TODO: This can probably be optimized a lot
    mat4 TranslationTransform = Mat4GetTranslation(Position);
    mat4 RotationTransform = Mat4GetRotationFromQuat(Rotation);
    mat4 ScaleTransform = Mat4GetScale(Scale);
    
    mat4 Result = TranslationTransform * RotationTransform * ScaleTransform;
    return Result;
}

inline vec3
FullTransformPoint(vec3 Point, vec3 Position, quat Rotation, vec3 Scale)
{
    vec3 Result = Point;
    
    Result = Mat3GetScale(Scale) * Result;
    Result = RotateVecByQuatSlow(Result, Rotation);
    Result = Result + Position;

    return Result;
}

inline vec3
TransformNormal(vec3 Normal, quat Rotation, vec3 Scale)
{
    vec3 Result = Normal;

    mat3 InverseScale = Mat3GetScale(1.0f / Scale);
    Result = InverseScale * Result;
    Result = RotateVecByQuatSlow(Result, Rotation);
    Result = VecNormalize(Result);

    return Result;
}

inline void
FullTransformPoint(vec3 *Point, vec3 Position, quat Rotation, vec3 Scale)
{
    *Point = FullTransformPoint(*Point, Position, Rotation, Scale);
}

inline void
TransformNormal(vec3 *Normal, quat Rotation, vec3 Scale)
{
    *Normal = TransformNormal(*Normal, Rotation, Scale);
}

inline mat4
Mat4GetView(vec3 Position, vec3 Front, vec3 Right, vec3 Up, f32 ThirdPersonRadius)
{
    mat4 Result = Mat4(1.0f);
    
    Result.E[0][0] =  Right.X;
    Result.E[0][1] =  Up.X;
    Result.E[0][2] = -Front.X;
    Result.E[1][0] =  Right.Y;
    Result.E[1][1] =  Up.Y;
    Result.E[1][2] = -Front.Y;
    Result.E[2][0] =  Right.Z;
    Result.E[2][1] =  Up.Z;
    Result.E[2][2] = -Front.Z;
    Result.E[3][0] = -VecDot(Right, Position);
    Result.E[3][1] = -VecDot(Up, Position);
    Result.E[3][2] =  VecDot(Front, Position) - ThirdPersonRadius;

    return Result;
}

inline vec3
GetCartesianVecFromYawPitch(f32 Yaw, f32 Pitch)
{
    f32 CosYaw = CosF(ToRadiansF(Yaw));
    f32 CosPitch = CosF(ToRadiansF(Pitch));
    f32 SinYaw = SinF(ToRadiansF(Yaw));
    f32 SinPitch = SinF(ToRadiansF(Pitch));

    vec3 Result = {};

    Result.X = CosPitch * SinYaw;
    Result.Y = SinPitch;
    Result.Z = CosPitch * CosYaw;

    return Result;
}

inline mat4
Mat4GetView(vec3 Position, f32 Yaw, f32 Pitch, f32 ThirdPersonRadius)
{
    vec3 Front = GetCartesianVecFromYawPitch(Yaw, Pitch);
    vec3 Right = VecNormalize(VecCross(Front, Vec3(0,1,0)));
    vec3 Up = VecCross(Right, Front);

    mat4 Result = Mat4GetView(Position, Front, Right, Up, ThirdPersonRadius);
    return Result;
}

inline mat4
Mat4GetPerspecitveProjection(f32 FovY_Degrees, f32 AspectRatio, f32 Near, f32 Far)
{
    // NOTE: http://www.songho.ca/opengl/gl_projectionmatrix.html
    mat4 Result = {};

    f32 HalfHeight = Near * TanF(ToRadiansF(FovY_Degrees) / 2.0f);
    f32 HalfWidth = HalfHeight * AspectRatio;

    Result.E[0][0] = Near / HalfWidth;
    Result.E[1][1] = Near / HalfHeight;
    Result.E[2][2] = -(Far + Near) / (Far - Near);
    Result.E[2][3] = -1.0f;
    Result.E[3][2] = -2.0f * Far * Near / (Far - Near);

    return Result;
}

inline mat4
Mat4GetOrthographicProjection(f32 Left, f32 Right, f32 Bottom, f32 Top, f32 Near, f32 Far)
{
    // NOTE: http://www.songho.ca/opengl/gl_projectionmatrix.html
    mat4 Result = {};

    Result.E[0][0] = 2.0f / (Right - Left);
    Result.E[1][1] = 2.0f / (Top - Bottom);
    Result.E[2][2] = -2.0f / (Far - Near);
    
    Result.E[3][0] = (Right + Left) / (Left - Right);
    Result.E[3][1] = (Top + Bottom) / (Bottom - Top);
    Result.E[3][2] = (Far + Near) / (Near - Far);
    Result.E[3][3] = 1.0f;

    return Result;
}

inline mat4
Mat4GetCamera2DView(vec2 Target, f32 S, f32 Rot, vec2 Offset)
{
    /*
     * This is this but hardcoded:
       mat4 Translation = Mat4GetTranslation(-Vec3(Target));
       mat4 Scale = Mat4GetScale(Vec3(S, S, 1.0f));
       mat4 Rotation = Mat4(Mat3GetRotationAroundAxis(Vec3(0,0,1), ToRadiansF(Rot)));
       mat4 Offset = Mat4GetTranslation(Vec3(Offset));
       mat4 Calc = Offset * Rotation * Scale * Translation;
    */

    mat4 Result = Mat4(1.0f);
    f32 CosRot = CosF(ToRadiansF(Rot));
    f32 SinRot = SinF(ToRadiansF(Rot));
    vec2 T = -Target;
    
    Result.E[0][0] = S * CosRot;
    Result.E[0][1] = S * SinRot;
    Result.E[1][0] = -S * SinRot;
    Result.E[1][1] = S * CosRot;
    Result.E[3][0] = S * T.X * CosRot - S * T.Y * SinRot + Offset.X;
    Result.E[3][1] = S * T.X * SinRot + S * T.Y * CosRot + Offset.Y;
 
    return Result;
}

inline mat4
Mat4GetCamera2DViewInv(vec2 Target, f32 S, f32 Rot, vec2 Offset)
{
    /*
     * This is this but hardcoded:
       mat4 OffsetInv = Mat4GetTranslation(-Vec3(Offset));
       mat4 RotationInv = Mat4(Mat3GetRotationAroundAxis(Vec3(0,0,1), ToRadiansF(-Rot)));
       mat4 ScaleInv = Mat4GetScale(Vec3(1.0f/S, 1.0f.S, 1.0f));
       mat4 TranslationInv = Mat4GetTranslation(Vec3(Target));
       mat4 CalcInv = TranslationInv * ScaleInv * RotationInv * OffsetInv;
    */

    mat4 Result = Mat4(1.0f);
    f32 CosRot = CosF(ToRadiansF(Rot));
    f32 SinRot = SinF(ToRadiansF(Rot));
    vec2 T = -Target;
    f32 ooS = 1.0f / S;
    
    Result.E[0][0] = ooS * CosRot;
    Result.E[0][1] = -ooS * SinRot;
    Result.E[1][0] = ooS * SinRot;
    Result.E[1][1] = ooS * CosRot;
    Result.E[3][0] = -Offset.X * CosRot * ooS - Offset.Y * SinRot * ooS - T.X;
    Result.E[3][1] = Offset.X * SinRot * ooS - Offset.Y * CosRot * ooS - T.Y;
 
    return Result;
}

inline mat4
Mat4GetCamera2DViewInvRel(f32 S, f32 Rot)
{
    /*
     * This is this but hardcoded:
       mat4 RotationInv = Mat4(Mat3GetRotationAroundAxis(Vec3(0,0,1), ToRadiansF(-Rot)));
       mat4 ScaleInv = Mat4GetScale(Vec3(1.0f/S, 1.0f.S, 1.0f));
       mat4 CalcInv = ScaleInv * RotationInv;
    */

    mat4 Result = Mat4(1.0f);
    f32 CosRot = CosF(ToRadiansF(Rot));
    f32 SinRot = SinF(ToRadiansF(Rot));
    f32 ooS = 1.0f / S;
    
    Result.E[0][0] = ooS * CosRot;
    Result.E[0][1] = -ooS * SinRot;
    Result.E[1][0] = ooS * SinRot;
    Result.E[1][1] = ooS * CosRot;
 
    return Result;
}

inline void
Rotate4PointsAroundOrigin(vec2 *Positions, vec2 Origin, f32 Rotation)
{
    f32 C = CosF(ToRadiansF(Rotation));
    f32 S = SinF(ToRadiansF(Rotation));

    for (int i = 0; i < 4; i++)
    {
        Positions[i] -= Origin;
        f32 X = Positions[i].X;
        f32 Y = Positions[i].Y;
        Positions[i] = Vec2(C*X - S*Y, S*X + C*Y);
        Positions[i] += Origin;
    }
}

inline void
Rotate4PointsAroundOrigin(vec3 *Positions, vec3 Origin, f32 Rotation)
{
    f32 C = CosF(ToRadiansF(Rotation));
    f32 S = SinF(ToRadiansF(Rotation));

    for (int i = 0; i < 4; i++)
    {
        Positions[i] -= Origin;
        f32 X = Positions[i].X;
        f32 Y = Positions[i].Y;
        Positions[i] = Vec3(C*X - S*Y, S*X + C*Y, 0.0f);
        Positions[i] += Origin;
    }
}


// -------------------------------------------------------------------------------
// VECTOR 2 INTEGER --------------------------------------------------------------
// -------------------------------------------------------------------------------
union vec2i
{
    struct
    {
        i32 X, Y;
    };

    i32 E[2];
};

inline vec2i Vec2I(i32 X, i32 Y) { vec2i Result = {}; Result.X = X; Result.Y = Y; return Result; }
inline vec2i Vec2I() { return Vec2I(0, 0); }
inline vec2i Vec2I(i32 Value) { return Vec2I(Value, Value); }
inline vec2i Vec2I(vec2 V_F) { return Vec2I((int) V_F.X, (int) V_F.Y); }
inline vec2 Vec2(vec2i V_I) { return Vec2((f32) V_I.X, (f32) V_I.Y); }
inline vec2i operator+(vec2i A, vec2i B) { return Vec2I(A.X + B.X, A.Y + B.Y); }
inline vec2i operator-(vec2i A, vec2i B) { return Vec2I(A.X - B.X, A.Y - B.Y); }
inline vec2i operator-(vec2i V) { return Vec2I(-V.X, -V.Y); }
inline vec2i operator*(vec2i A, vec2i B) { return Vec2I(A.X * B.X, A.Y * B.Y); }
inline vec2i operator*(vec2i V, i32 I) { return Vec2I(V.X * I, V.Y * I); }
inline vec2i operator*(vec2i V, f32 I) { return Vec2I((i32) (V.X * I), (i32) (V.Y * I)); }
inline vec2i operator*(i32 I, vec2i V) { return Vec2I(V.X * I, V.Y * I); }
inline vec2i operator*(f32 I, vec2i V) { return Vec2I((i32) (V.X * I), (i32) (V.Y * I)); }
inline vec2i operator/(vec2i V, i32 I) { return Vec2I(V.X / I, V.Y / I); }
inline vec2i operator/(vec2i V, f32 I) { return Vec2I((i32) (V.X / I), (i32) (V.Y / I)); }
inline vec2i operator/(i32 I, vec2i V) { return Vec2I(I / V.X, I / V.Y); }
inline vec2i operator/(f32 I, vec2i V) { return Vec2I((i32) (I / V.X), (i32) (I / V.Y)); }
inline vec2i &operator+=(vec2i &V0, vec2i V1) { V0 = V0 + V1; return V0; }
inline vec2i &operator-=(vec2i &V0, vec2i V1) { V0 = V0 - V1; return V0; }
inline vec2i &operator*=(vec2i &V, int S) { V = V * S; return V; }
inline vec2i &operator/=(vec2i &V, int S) { V = V / S; return V; }
inline b32 operator==(vec2i A, vec2i B) { return (A.X == B.X && A.Y == B.Y); }
inline int VecLengthSq(vec2i V) { return (V.X * V.X + V.Y * V.Y); }

// -------------------------------------------------------------------------------
// VECTOR 3 INTEGER --------------------------------------------------------------
// -------------------------------------------------------------------------------
union vec3i
{
    struct
    {
        i32 X, Y, Z;
    };

    i32 E[3];
};

inline vec3i Vec3I(i32 X, i32 Y, i32 Z) { vec3i Result = {}; Result.X = X; Result.Y = Y; Result.Z = Z; return Result; }
inline vec3i Vec3I() { return Vec3I(0, 0, 0); }
inline vec3i Vec3I(i32 Value) { return Vec3I(Value, Value, Value); }
inline vec3i Vec3I(vec2i V) { return Vec3I(V.X, V.Y, 0); }
inline vec2i Vec2I(vec3i V) { return Vec2I(V.X, V.Y); }
inline b32 operator==(vec3i A, vec3i B) { return (A.X == B.X && A.Y == B.Y && A.Z == B.Z); }
inline vec3i operator+(vec3i A, vec3i B) { return Vec3I(A.X + B.X, A.Y + B.Y, A.Z + B.Z); }
inline vec3i operator-(vec3i A, vec3i B) { return Vec3I(A.X - B.X, A.Y - B.Y, A.Z - B.Z); }
inline vec3i operator*(vec3i A, vec3i B) { return Vec3I(A.X * B.X, A.Y * B.Y, A.Z * B.Z); }

#endif
