// defines.h - defines basic stuff that almost everything needs
//

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>

#ifndef byte
#define byte unsigned char
#endif  // byte

#ifndef dword
#define dword unsigned int
#endif  // DWORD

// from MSFT version of stdlib.h, not all version of stdlib.h have this useful macro
#if !defined(_countof)
#define _countof(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif

// taken from math.h (not part of the normal include)
#define M_PI				3.14159265358979323846

#define	EPSILON				0.00001

// namespaces
#define _MIGRENDER_BEGIN    namespace MigRender {
#define _MIGRENDER_END      }

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// enums
//-----------------------------------------------------------------------------

// image format
enum class ImageFormat
{
	None, RGBA, BGRA, RGB, BGR, GreyScale, Bump
};

// image resizing
enum class ImageResize
{
	None, Stretch, ScaleToFit, ScaleToFill
};

// texture filtering
enum class TextureFilter
{
	None, Nearest, Bilinear
};

// texture mapping types
enum class TextureMapType
{
	None, Diffuse, Specular, Refraction, Glow, Reflection, Transparency, Bump
};

// texture mapping operations
enum class TextureMapOp
{
	Multiply, Add, Subtract, Blend
};

// texture wrap mapping types
enum class TextureMapWrapType
{
	None, Spherical, Elliptical, Projection, Extrusion
};

// supersampling types
enum class SuperSample
{
	None, X1, Edge, Object, X5, X9
};

// ray types
enum class RayType
{
	Origin, Reflect, Refract
};

// object types
enum class ObjType
{
	None, Camera, Bg, Group, DirLight, PtLight, SpotLight, Sphere, Polygon
};

//-----------------------------------------------------------------------------
// macros
//-----------------------------------------------------------------------------

// Minimum and maximum macros
#ifndef max
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif  // max
#ifndef min
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif  // min

//-----------------------------------------------------------------------------
// COLOR - defines a complete color, with alpha blending
//-----------------------------------------------------------------------------

class COLOR
{
public:
	double r;
	double g;
	double b;
	double a;

public:
	COLOR()
		{ r = g = b = a = 0; };
	COLOR(double c)
		{ r = c; g = c; b = c; a = 0; }
	COLOR(double _r, double _g, double _b)
		{ r = _r; g = _g; b = _b; a = 0; }
	COLOR(double _r, double _g, double _b, double _a)
		{ r = _r; g = _g; b = _b; a = _a; }
	COLOR(const COLOR& rhs)
		{ r = rhs.r; g = rhs.g; b = rhs.b; a = rhs.a; }

	void Init(void)
		{ r = g = b = a = 0; }
	void Init(double c)
		{ r = g = b = c; a = 0; }
	void Init(double _r, double _g, double _b)
		{ r = _r; g = _g; b = _b; a = 0; }
	void Init(double _r, double _g, double _b, double _a)
		{ r = _r; g = _g; b = _b; a = _a; }

	void Invert(void)
		{ r = 1 - r; g = 1 - g; b = 1 - b; a = 1 - a; }

	void operator+=(const COLOR& rhs)
		{ r += rhs.r; g += rhs.g; b += rhs.b; }
	void operator-=(const COLOR& rhs)
		{ r -= rhs.r; g -= rhs.g; b -= rhs.b; }
	void operator*=(double rhs)
		{ r *= rhs; g *= rhs; b *= rhs; }
	void operator*=(const COLOR& rhs)
		{ r *= rhs.r; g *= rhs.g; b *= rhs.b; a *= rhs.a; }
	void operator/=(double rhs)
		{ r /= rhs; g /= rhs; b /= rhs; }

	COLOR operator+(const COLOR& rhs) const
		{ return COLOR(r + rhs.r, g + rhs.g, b + rhs.b, rhs.a); }
	COLOR operator-(const COLOR& rhs) const
		{ return COLOR(r - rhs.r, g - rhs.g, b - rhs.b, rhs.a); }
	COLOR operator*(const COLOR& rhs) const
		{ return COLOR(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }
};
typedef class COLOR* PCOLOR;

//-----------------------------------------------------------------------------
// UVC - defines a texture map UV coordinate
//-----------------------------------------------------------------------------

class UVC
{
public:
	double u;
	double v;

public:
	UVC()
		{ u = v = 0; };
	UVC(double _u, double _v)
		{ u = _u; v = _v; }
	UVC(const UVC& rhs)
		{ u = rhs.u; v = rhs.v; }

	UVC operator+(const UVC& rhs) const
		{ return UVC(u + rhs.u, v + rhs.v); }
	UVC operator-(const UVC& rhs) const
		{ return UVC(u - rhs.u, v - rhs.v); }
	UVC operator*(double factor) const
		{ return UVC(u*factor, v*factor); }
};

#define UVMIN UVC(0, 0)
#define UVMAX UVC(1, 1)

_MIGRENDER_END
