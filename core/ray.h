// ray.h - defines a ray for the ray tracer
//

#pragma once

#include "vector.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// SCREENPOS - defines a point on the screen
//-----------------------------------------------------------------------------

class SCREENPOS
{
public:
	int x;
	int y;
	bool valid;

public:
	SCREENPOS()
		{ x = y = 0; valid = false; };
	SCREENPOS(int _x, int _y)
		{ x = _x; y = _y; valid = true; }
	SCREENPOS(const SCREENPOS& rhs)
		{ x = rhs.x; y = rhs.y; valid = rhs.valid; }

	void Init(void)
		{ x = y = 0; valid = false; }
	void Init(int _x, int _y)
		{ x = _x; y = _y; valid = true; }

	bool IsValid(void) const
		{ return valid; }
};

//-----------------------------------------------------------------------------
// CRay - a single ray in space
//
//  The ray is defined as an origin point and a direction unit vector.
//-----------------------------------------------------------------------------

class CRay
{
public:
	CPt ori;
	CUnitVector dir;

	COLOR str;
	int gen;
	bool in;

	SuperSample ssmpl;
	SCREENPOS pos;
	RayType type;

public:
	CRay(void);
	CRay(const CRay& ray);
	CRay(const CPt& _ori, const CUnitVector& _dir);

	void OffsetOriginByEpsilon(void);
};

_MIGRENDER_END
