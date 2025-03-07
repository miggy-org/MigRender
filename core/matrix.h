// matrix.h - defines transformation matricies
//

#pragma once

#include "defines.h"
#include "ray.h"

_MIGRENDER_BEGIN

enum class MatrixType
{
	None = 0,
	CTM,
	ICTM,
	ITCTM
};

class CMatrix_f;

//-----------------------------------------------------------------------------
// CMatrix - a transformation matrix
//
//  This class also holds the complete inverse and inverse transpose form
//  of the original matrix.  Before using those you need to call
//  ComputeInverseTranspose(), and changing the matrix will not automatically
//  update those two additional matricies.
//-----------------------------------------------------------------------------

class CMatrix
{
protected:
	double m_ctm[16];
	double m_ictm[16];
	double m_itctm[16];

	bool m_identity;

protected:
	void CheckIdentity(void);

public:
	CMatrix(void);
	CMatrix(const CMatrix& rhs);
	CMatrix(const CMatrix_f& rhs);
	~CMatrix(void);

	CMatrix& SetIdentity(void);
	bool IsIdentity(void) const;
	const double* GetCTM(void) const;

	const double& operator[](int index) const;
	double& operator[](int index);

	bool ComputeInverseTranspose(void);

	bool TransformPoint(CPt& pt, double fw, MatrixType type) const;
	double TransformRay(CRay& ray, MatrixType type) const;

	CMatrix& MatrixMultiply(const CMatrix& rhs);

	CMatrix& Translate(double fx, double fy, double fz);
	CMatrix& Scale(double fx, double fy, double fz);
	CMatrix& RotateX(double fx, bool rad = false);
	CMatrix& RotateY(double fy, bool rad = false);
	CMatrix& RotateZ(double fz, bool rad = false);
	CMatrix& ShearXY(double fx, double fy);
	CMatrix& ShearXZ(double fx, double fz);
	CMatrix& ShearYZ(double fy, double fz);
};

//-----------------------------------------------------------------------------
// CMatrix_f - smaller version of the matrix, for file loading/saving
//-----------------------------------------------------------------------------

class CMatrix_f
{
public:
	double m_ctm[16];

public:
	CMatrix_f(void);
	CMatrix_f(const CMatrix& rhs);

	operator const double*() const;
};

_MIGRENDER_END
