// vector.cpp - defines simple points, vectors and unit vectors
//

#include <math.h>
#include "defines.h"
#include "vector.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CPt
//-----------------------------------------------------------------------------

CPt::CPt(void)
{
	x = y = z = 0;
}

CPt::CPt(const CPt& p)
{
	x = p.x;
	y = p.y;
	z = p.z;
}

CPt::CPt(double _x, double _y, double _z)
{
	x = _x;
	y = _y;
	z = _z;
}

void CPt::SetPoint(double _x, double _y, double _z)
{
	x = _x;
	y = _y;
	z = _z;
}

void CPt::Scale(double l)
{
	x *= l;
	y *= l;
	z *= l;
}

void CPt::Invert(void)
{
	Scale(-1);
}

double CPt::GetLength(void) const
{
	return sqrt(x*x + y*y + z*z);
}

bool CPt::operator==(const CPt& rhs) const
{
	return ((x == rhs.x && y == rhs.y && z == rhs.z) ? true : false);
}

CPt CPt::operator+(const CPt& rhs) const
{
	return CPt(x + rhs.x, y + rhs.y, z + rhs.z);
}

CPt CPt::operator-(const CPt& rhs) const
{
	return CPt(x - rhs.x, y - rhs.y, z - rhs.z);
}

CPt CPt::operator*(double rhs) const
{
	return CPt(x*rhs, y*rhs, z*rhs);
}

CPt CPt::operator/(double rhs) const
{
	return CPt(x/rhs, y/rhs, z/rhs);
}

//-----------------------------------------------------------------------------
// CUnitVector
//-----------------------------------------------------------------------------

CUnitVector::CUnitVector(void)
{
}

CUnitVector::CUnitVector(const CUnitVector& uv)
{
	x = uv.x;
	y = uv.y;
	z = uv.z;
}

CUnitVector::CUnitVector(const CPt& pt)
{
	x = pt.x;
	y = pt.y;
	z = pt.z;
}

CUnitVector::CUnitVector(double _x, double _y, double _z)
{
	x = _x;
	y = _y;
	z = _z;
}

CUnitVector CUnitVector::CrossProduct(const CUnitVector& rhs) const
{
	CUnitVector ret(y*rhs.z - z*rhs.y, z*rhs.x - x*rhs.z, x*rhs.y - y*rhs.x);
	return ret;
}

double CUnitVector::DotProduct(const CUnitVector& rhs) const
{
	return (x*rhs.x + y*rhs.y + z*rhs.z);
}

double CUnitVector::Normalize(void)
{
    double len = GetLength();
    if (len != 0.0)
		Scale(1.0 / len);
    return len;
}

bool CUnitVector::operator==(const CUnitVector& rhs) const
{
	return ((x == rhs.x && y == rhs.y && z == rhs.z) ? true : false);
}

CUnitVector CUnitVector::operator+(const CUnitVector& rhs) const
{
	return CUnitVector(x + rhs.x, y + rhs.y, z + rhs.z);
}

CUnitVector CUnitVector::operator-(const CUnitVector& rhs) const
{
	return CUnitVector(x - rhs.x, y - rhs.y, z - rhs.z);
}

//-----------------------------------------------------------------------------
// CVector
//-----------------------------------------------------------------------------

CVector::CVector(void)
{
}

CVector::CVector(const CVector& v)
{
	ori = v.ori;
	dir = v.dir;
}

CVector::CVector(const CPt& _ori, const CPt& _dir)
{
	ori = _ori;
	dir = _dir;
}

void CVector::SetVector(const CPt& _ori, const CPt& _dir)
{
	ori = _ori;
	dir = _dir;
}

bool CVector::operator==(const CVector& rhs) const
{
	return ((ori == rhs.ori && dir == rhs.dir) ? true : false);
}
