// ray.cpp - defines a ray for the ray tracer
//

#include "ray.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CRay
//-----------------------------------------------------------------------------

CRay::CRay(void)
{
	gen = 1;
	in = false;
	ssmpl = SuperSample::X1;
	pos.Init();
	type = RayType::Origin;
}

CRay::CRay(const CRay& ray)
{
	ori = ray.ori;
	dir = ray.dir;
	str = ray.str;
	gen = ray.gen;
	in = ray.in;
	ssmpl = ray.ssmpl;
	pos = ray.pos;
	type = ray.type;
}

CRay::CRay(const CPt& _ori, const CUnitVector& _dir)
{
	ori = _ori;
	dir = _dir;
	str.Init(1);
	gen = 0;
	in = false;
	ssmpl = SuperSample::X1;
	pos.Init();
	type = RayType::Origin;
}

// offsets the origin by a small amount, usually used to avoid having
//  reflection/refraction rays intersect the objects that created them
//  in the first place
void CRay::OffsetOriginByEpsilon(void)
{
	ori.x += EPSILON*dir.x;
	ori.y += EPSILON*dir.y;
	ori.z += EPSILON*dir.z;
}
