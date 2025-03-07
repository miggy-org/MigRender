// matrix.cpp - defines transformation matricies
//

#include <memory.h>
#include <stdexcept>
#include "matrix.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CMatrix
//-----------------------------------------------------------------------------

CMatrix::CMatrix(void)
{
	SetIdentity();
	memset(m_ictm, 0, 16*sizeof(double));
	memset(m_itctm, 0, 16*sizeof(double));
}

CMatrix::CMatrix(const CMatrix& rhs)
{
	memcpy(m_ctm, rhs.m_ctm, 16*sizeof(double));
	memcpy(m_ictm, rhs.m_ictm, 16*sizeof(double));
	memcpy(m_itctm, rhs.m_itctm, 16*sizeof(double));
	m_identity = rhs.m_identity;
}

CMatrix::CMatrix(const CMatrix_f& rhs)
{
	memcpy(m_ctm, rhs.m_ctm, 16*sizeof(double));
	memset(m_ictm, 0, 16*sizeof(double));
	memset(m_itctm, 0, 16*sizeof(double));
	CheckIdentity();
}

CMatrix::~CMatrix(void)
{
}

void CMatrix::CheckIdentity(void)
{
	m_identity = true;
	for (int n = 0; m_identity && n < 16; n++)
	{
		if (n == 0 || n == 5 || n == 10 || n == 15)
			m_identity = (m_ctm[n] == 1);
		else
			m_identity = (m_ctm[n] == 0);
	}
}

CMatrix& CMatrix::SetIdentity(void)
{
	memset(m_ctm, 0, 16*sizeof(double));
	m_ctm[0] = m_ctm[5] = m_ctm[10] = m_ctm[15] = 1;
	m_identity = true;
	return *this;
}

bool CMatrix::IsIdentity(void) const
{
	return m_identity;
}

const double* CMatrix::GetCTM(void) const
{
	return m_ctm;
}

const double& CMatrix::operator[](int index) const
{
	if (index < 0 || index >= 16)
		throw std::out_of_range("Matrix index out of bounds");
	return m_ctm[index];
}

double& CMatrix::operator[](int index)
{
	if (index < 0 || index >= 16)
		throw std::out_of_range("Matrix index out of bounds");
	m_identity = false;
	return m_ctm[index];
}

bool CMatrix::ComputeInverseTranspose(void)
{
	if (!IsIdentity())
	{
		double tmp[32], val;
		int i, j, n;

		// set up the matrix (and round off values)
		for (j = 0; j < 4; j++)
		{
			for (i = 0; i < 4; i++)
			{
				tmp[8*j+i] = (fabs(m_ctm[4*j+i]) < EPSILON ? 0.0 : m_ctm[4*j+i]);
				tmp[8*j+i+4] = (i == j ? 1.0 : 0.0);
			}
		}

		// compute the inverse of the CTM using the Gaussian elimination method
		for (j = 0; j < 4; j++)
		{
			// check to see if diagonal value is zero and if so compensate
			if (tmp[8*j+j] == 0)
			{
				for (i = j + 1; i < 4; i++)
				{
					if (tmp[8*i+j] != 0)
					{
						val = 1.0 / tmp[8*i+j];
						for (n = 0; n < 8; n++)
						{
							tmp[8*j+n] += val*tmp[8*i+n];
							tmp[8*j+n] = (fabs(tmp[8*j+n]) < EPSILON ? 0.0 : tmp[8*j+n]);
						}

						break;
					}
				}

				if (i == 4)
					return false;
			}

			// scale down row to 1
			for (i = 0; i < 8; i++)
			{
				if (i != j)
					tmp[8*j+i] /= tmp[8*j+j];
			}
			tmp[8*j+j] = 1.0;

			// set columns to 0
			for (i = 0; i < 4; i++)
			{
				if (i != j && tmp[8*i+j] != 0)
				{
					val = -tmp[8*i+j];
					for (n = 0; n < 8; n++)
					{
						tmp[8*i+n] += val*tmp[8*j+n];
						tmp[8*i+n] = (fabs(tmp[8*i+n]) < EPSILON ? 0.0 : tmp[8*i+n]);
					}
				}
			}
		}

		// transpose the result and set final matricies
		for (j = 0; j < 4; j++)
		{
			for (i = 0; i < 4; i++)
			{
				m_itctm[4*j+i] = tmp[8*i+j+4];
				m_ictm[4*j+i] = tmp[8*j+i+4];
			}
		}
	}

	return true;
}

bool CMatrix::TransformPoint(CPt& pt, double fw, MatrixType type) const
{
	if (IsIdentity())
		return true;

	const double *ptm;
	switch (type)
	{
	case MatrixType::CTM: ptm = m_ctm; break;
	case MatrixType::ICTM: ptm = m_ictm; break;
	case MatrixType::ITCTM: ptm = m_itctm; break;
	default: return false;
	}

	double tmp[4];
	for (int j = 0; j < 4; j++)
	{
		tmp[j] = ptm[4*j]*pt.x;
		tmp[j] += ptm[4*j+1]*pt.y;
		tmp[j] += ptm[4*j+2]*pt.z;
		tmp[j] += ptm[4*j+3]*fw;
	}

	// NOTE: there's a discrepency between this implementation and that of the
	//  CS80 project if tmp[3] == -1, check against CS172 code
	pt.SetPoint(tmp[0], tmp[1], tmp[2]);
	if (tmp[3] < 0)
	{
		double scale = -1.0 / tmp[3];
		pt.Scale(scale);
	}
	else if (tmp[3] > 0 && tmp[3] < 1)
	{
		double scale = 1.0 / tmp[3];
		pt.Scale(scale);
	}

	return true;
}

double CMatrix::TransformRay(CRay& ray, MatrixType type) const
{
	if (IsIdentity())
		return 1;

	TransformPoint(ray.ori, 1.0, type);
	TransformPoint(ray.dir, 0.0, type);

	// return the scale factor between the new and original rays
	double len = 1.0 / ray.dir.GetLength();
	ray.dir.Scale(len);
	return len;
}

CMatrix& CMatrix::MatrixMultiply(const CMatrix& rhs)
{
	if (!IsIdentity() || !rhs.IsIdentity())
	{
		double ctm[16];

		for (int j = 0; j < 4; j++)
		{
			for (int i = 0; i < 4; i++)
			{
				ctm[4*j+i] = 0.0;

				for (int k = 0; k < 4; k++)
				{
					ctm[4*j+i] += rhs.m_ctm[4*j+k]*m_ctm[4*k+i];
				}
			}
		}

		memcpy(m_ctm, ctm, 16*sizeof(double));
		m_identity = false;
	}
	return *this;
}

CMatrix& CMatrix::Translate(double fx, double fy, double fz)
{
	if (fx != 0 || fy != 0 || fz != 0)
	{
		CMatrix tmp;
		tmp.m_ctm[3] = fx;
		tmp.m_ctm[7] = fy;
		tmp.m_ctm[11] = fz;
		tmp.m_identity = false;
		MatrixMultiply(tmp);
	}
	return *this;
}

CMatrix& CMatrix::Scale(double fx, double fy, double fz)
{
	if (fx != 1 || fy != 1 || fz != 1)
	{
		CMatrix tmp;
		tmp.m_ctm[0] = fx;
		tmp.m_ctm[5] = fy;
		tmp.m_ctm[10] = fz;
		tmp.m_identity = false;
		MatrixMultiply(tmp);
	}
	return *this;
}

CMatrix& CMatrix::RotateX(double fx, bool rad)
{
	if (fx != 0)
	{
		if (!rad)
			fx = M_PI*fx / 180.0;

		CMatrix tmp;
		tmp.m_ctm[5] = cos(fx);
		tmp.m_ctm[6] = -sin(fx);
		tmp.m_ctm[9] = sin(fx);
		tmp.m_ctm[10] = cos(fx);
		tmp.m_identity = false;
		MatrixMultiply(tmp);
	}
	return *this;
}

CMatrix& CMatrix::RotateY(double fy, bool rad)
{
	if (fy != 0)
	{
		if (!rad)
			fy = M_PI*fy / 180.0;

		CMatrix tmp;
		tmp.m_ctm[0] = cos(fy);
		tmp.m_ctm[2] = sin(fy);
		tmp.m_ctm[8] = -sin(fy);
		tmp.m_ctm[10] = cos(fy);
		tmp.m_identity = false;
		MatrixMultiply(tmp);
	}
	return *this;
}

CMatrix& CMatrix::RotateZ(double fz, bool rad)
{
	if (fz != 0)
	{
		if (!rad)
			fz = M_PI*fz / 180.0;

		CMatrix tmp;
		tmp.m_ctm[0] = cos(fz);
		tmp.m_ctm[1] = -sin(fz);
		tmp.m_ctm[4] = sin(fz);
		tmp.m_ctm[5] = cos(fz);
		tmp.m_identity = false;
		MatrixMultiply(tmp);
	}
	return *this;
}

CMatrix& CMatrix::ShearXY(double fx, double fy)
{
	CMatrix tmp;
	tmp.m_ctm[2] = fx;
	tmp.m_ctm[6] = fy;
	tmp.m_identity = false;
	return MatrixMultiply(tmp);
}

CMatrix& CMatrix::ShearXZ(double fx, double fz)
{
	CMatrix tmp;
	tmp.m_ctm[4] = fx;
	tmp.m_ctm[8] = fz;
	tmp.m_identity = false;
	return MatrixMultiply(tmp);
}

CMatrix& CMatrix::ShearYZ(double fy, double fz)
{
	CMatrix tmp;
	tmp.m_ctm[1] = fy;
	tmp.m_ctm[9] = fz;
	tmp.m_identity = false;
	return MatrixMultiply(tmp);
}

//-----------------------------------------------------------------------------
// CMatrix_f
//-----------------------------------------------------------------------------

CMatrix_f::CMatrix_f(void)
{
	memset(m_ctm, 0, 16 * sizeof(double));
}

CMatrix_f::CMatrix_f(const CMatrix& rhs)
{
	memcpy(m_ctm, rhs.GetCTM(), sizeof(m_ctm));
}

CMatrix_f::operator const double*() const
{
	return m_ctm;
}
