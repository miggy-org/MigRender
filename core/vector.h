// vector.h - defines simple points, vectors and unit vectors
//

#pragma once

#include "defines.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CPt - defines a point in space
//-----------------------------------------------------------------------------

class CPt
{
public:
	double x, y, z;

public:
	CPt(void);
	CPt(const CPt& p);
	CPt(double _x, double _y, double _z);

	void SetPoint(double _x, double _y, double _z);
	void Scale(double l);
	void Invert(void);
	double GetLength(void) const;

	bool operator==(const CPt& rhs) const;
	CPt operator+(const CPt& rhs) const;
	CPt operator-(const CPt& rhs) const;
	CPt operator*(double rhs) const;
	CPt operator/(double rhs) const;
};

//-----------------------------------------------------------------------------
// CUnitVector - defines a unit vector (direction point only, origin is assumed)
//-----------------------------------------------------------------------------

class CUnitVector : public CPt
{
public:
	CUnitVector(void);
	CUnitVector(const CPt& pt);
	CUnitVector(const CUnitVector& uv);
	CUnitVector(double _x, double _y, double _z);

	CUnitVector CrossProduct(const CUnitVector& rhs) const;
	double DotProduct(const CUnitVector& rhs) const;
	double Normalize(void);

	bool operator==(const CUnitVector& rhs) const;
	CUnitVector operator+(const CUnitVector& rhs) const;
	CUnitVector operator-(const CUnitVector& rhs) const;
};

//-----------------------------------------------------------------------------
// CVector - defines a vector (origin and direction points)
//-----------------------------------------------------------------------------

class CVector
{
public:
	CPt ori;
	CPt dir;

public:
	CVector(void);
	CVector(const CVector& v);
	CVector(const CPt& _ori, const CPt& _dir);

	void SetVector(const CPt& _ori, const CPt& _dir);

	bool operator==(const CVector& rhs) const;
};

_MIGRENDER_END
