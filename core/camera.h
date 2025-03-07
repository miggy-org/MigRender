// camera.h - defines a camera
//

#pragma once

#include "vector.h"
#include "matrix.h"
#include "fileio.h"
#include "baseobject.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CCamera - defines a camera
//-----------------------------------------------------------------------------

class CCamera : public CBaseObj
{
protected:
	double m_dist;
	double m_ulen;
	double m_vlen;

	CMatrix m_ftm;

	int m_scrwidth;
	int m_scrheight;

	// temporary animation backup
	double m_distOrig;
	double m_ulenOrig;
	double m_vlenOrig;

public:
	CCamera(void);
	CCamera(const CCamera& cam);
	~CCamera(void);

	ObjType GetType(void) const { return ObjType::Camera; }

	void SetViewport(double ulen, double vlen, double dist);
	void GetViewport(double& ulen, double& vlen, double& dist) const;

	// file I/O
	void Load(CFileBase& fobj);
	void Save(CFileBase& fobj);

	// animation
	void PreAnimate(void);
	void PostAnimate(void);
	void ResetPlayback(void);
	void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	void RenderStart(int w, int h);
	void RenderFinish(void);

	bool GlobalToScreen(const CPt& pt, SCREENPOS& pos) const;
	bool GetTraceParams(CPt& ori, CPt& vp1, CPt& vp2, CPt& vp3, CPt& vp4) const;
};

_MIGRENDER_END
