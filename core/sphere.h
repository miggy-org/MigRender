// sphere.h - defines the sphere class
//

#pragma once

#include "object.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CSphere - defines a simple sphere
//-----------------------------------------------------------------------------

class CSphere : public CObj
{
protected:
	CPt m_ori;
	double m_rad, m_rad2;

	// temporary rendering variables (not thread safe)
	std::vector<CUnitVector> m_lnorms;

protected:
	// texture rendering
	virtual bool ComputeTexelCoord(int nthread, const CTexture* ptxt, UVC& final);

	// bounding box
	virtual void GetBoundBox(const CMatrix& tm, CBoundBox& bbox) const;

public:
	CSphere(void);
	virtual ~CSphere(void);

	ObjType GetType(void) const { return ObjType::Sphere; }

	void operator=(const CSphere& rhs);

	void SetOrigin(const CPt& ori);
	void SetRadius(double rad);

	const CPt& GetOrigin(void) const { return m_ori; }
	double GetRadius(void) const { return m_rad; }

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	virtual void RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images);
	virtual void RenderFinish(void);

	virtual bool IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr);
	virtual bool PostIntersect(int nthread, INTERSECTION& intr);
};

_MIGRENDER_END
