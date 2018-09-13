#pragma once

//////////////////////////////////////////////////////////////////////////
// Common C/C++ headers from the standard
//////////////////////////////////////////////////////////////////////////

#include <cstdint>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstring>


#include <algorithm>
#include <functional>

//////////////////////////////////////////////////////////////////////////
// Common Windows and directX Headers
//////////////////////////////////////////////////////////////////////////

#define NOIME
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl.h>
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>

//////////////////////////////////////////////////////////////////////////
// Maths related headers
//////////////////////////////////////////////////////////////////////////
#include <DirectXMath.h>
#include "DirectXTK/SimpleMath.h"

// ComPtr is useful for simplifying release of Com objects.
// see : https://github.com/Microsoft/DirectXTK/wiki/ComPtr

using Microsoft::WRL::ComPtr;

//////////////////////////////////////////////////////////////////////////
// Debug drawing library
//////////////////////////////////////////////////////////////////////////
#define DEBUG_DRAW_EXPLICIT_CONTEXT
#include "debug_draw/debug_draw.hpp"

//////////////////////////////////////////////////////////////////////////
// Imgui library
//////////////////////////////////////////////////////////////////////////
#include "imgui/imgui.h"

//////////////////////////////////////////////////////////////////////////
// Common game industry typedefs
//  * Very compact when used in expressions.
//  * Express the size in bytes.
//////////////////////////////////////////////////////////////////////////

// Unsigned
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

// Signed
using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

// Floating point
using f32 = float;
using f64 = double;

// Vector maths.
using v2 = DirectX::SimpleMath::Vector2;
using v3 = DirectX::SimpleMath::Vector3;
using v4 = DirectX::SimpleMath::Vector4;
using m4x4 = DirectX::SimpleMath::Matrix;
using quat = DirectX::SimpleMath::Quaternion;

//////////////////////////////////////////////////////////////////////////
// Useful assertion macro
//////////////////////////////////////////////////////////////////////////

#define ASSERT(x) if(!(x)){ __debugbreak(); }

#define SAFE_RELEASE(ptr) if(ptr){ ptr->Release(); }

// ========================================================
// Debug printing functions
// ========================================================

// Prints error to standard error stream.
void errorF(const char * format, ...);

// Printf to message box and abort
void panicF(const char * format, ...);

// Printf to console and debug output.
void debugF(const char * format, ...);


// ========================================================
// Frequently used maths
// ========================================================

constexpr f32 kfPI = 3.1415926535897931f;
constexpr f32 kfHalfPI = 0.5f * kfPI;
constexpr f32 kfTwoPI = 2.0f * kfPI;

// Angle in degrees to angle in radians
constexpr f32 degToRad(const f32 degrees)
{
	return degrees * kfPI / 180.0f;
}

// Angle in radians to angle in degrees
constexpr f32 radToDeg(const f32 radians)
{
	return radians * 180.0f / kfPI;
}

// Random numbers (0, 1) and (-1, 1) for floats and vectors.
inline f32 randf_norm() { return (float)rand() / RAND_MAX; }
inline f32 randf() { return randf_norm() * 2.0f - 1.0f; }
inline v2 randv2() { return v2(randf(), randf()); }
inline v3 randv3() { return v3(randf(), randf(), randf()); }
inline v4 randv4() { return v4(randf(), randf(), randf(), randf()); }