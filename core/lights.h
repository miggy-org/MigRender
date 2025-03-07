// lights.h - defines various light classes (directional, point, spot)
//

#pragma once

#include "vector.h"
#include "matrix.h"
#include "fileio.h"
#include "baseobject.h"

// light flags
#define LITF_NONE				0x00000000
#define LITF_SHADOW				0x00000001
#define LITF_SOFT_SHADOW		0x00000002

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CLight - a single light base class
//-----------------------------------------------------------------------------

class CLight : public CBaseObj
{
protected:
	COLOR m_col;
	double m_hlit;
	dword m_flags;
	double m_sscale;

	CMatrix m_ftm;

	// temporary animation
	COLOR m_colOrig;
	double m_hlitOrig;
	double m_sscaleOrig;

public:
	CLight(void);
	virtual ~CLight(void);

	virtual void SetColor(const COLOR& col);
	virtual const COLOR& GetColor(void) const;
	virtual void SetHighlight(double hlit);
	virtual double GetHighlight(void) const;
	virtual void SetShadowCaster(bool shadow, bool soft = false, double sscale = 1);
	virtual bool IsShadowCaster(void) const;
	virtual bool IsSoftShadow(void) const;
	virtual double GetSoftScale(void) const;

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	// animation
	virtual void PreAnimate(void);
	virtual void PostAnimate(void);
	virtual void ResetPlayback(void);
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	virtual void RenderStart(const CMatrix& itm);
	virtual void RenderFinish(void);

	virtual bool GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist) = 0;
	virtual bool DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
		COLOR& diffcol, COLOR& speccol) = 0;
};

//-----------------------------------------------------------------------------
// CDirLight - a directional light (origin is infinitely far away)
//-----------------------------------------------------------------------------

class CDirLight : public CLight
{
protected:
	CUnitVector m_dir, m_fdir;

public:
	CDirLight(void);

	ObjType GetType(void) const { return ObjType::DirLight; }

	void SetDirection(const CUnitVector& dir);
	const CUnitVector& GetDirection(void) const;

	virtual bool IsSoftShadow(void) const;

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	virtual void RenderStart(const CMatrix& itm);
	virtual bool GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist);
	virtual bool DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
		COLOR& diffcol, COLOR& speccol);
};

//-----------------------------------------------------------------------------
// CPtLight - a point light source
//-----------------------------------------------------------------------------

class CPtLight : public CLight
{
protected:
	CPt m_ori, m_fori;
	double m_fulldist;
	double m_dropdist;
	double m_droplen;

	// temporary animation
	double m_fulldistOrig;
	double m_dropdistOrig;

public:
	CPtLight(void);

	ObjType GetType(void) const { return ObjType::PtLight; }

	void SetOrigin(const CPt& ori);
	const CPt& GetOrigin(void) const;
	void SetDropoff(double full, double drop);
	double GetDropoffFull(void) const;
	double GetDropoffDrop(void) const;

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	// animation
	virtual void PreAnimate(void);
	virtual void PostAnimate(void);
	virtual void ResetPlayback(void);
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	virtual void RenderStart(const CMatrix& itm);
	virtual bool GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist);
	virtual bool DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
		COLOR& diffcol, COLOR& speccol);
};

//-----------------------------------------------------------------------------
// CSpotLight - a spot light
//-----------------------------------------------------------------------------

class CSpotLight : public CLight
{
protected:
	CPt m_ori, m_fori;
	CUnitVector m_dir, m_fdir;
	double m_concentration;
	double m_fulldist;
	double m_dropdist;
	double m_droplen;

	// temporary animation
	double m_fulldistOrig;
	double m_dropdistOrig;

public:
	CSpotLight(void);

	ObjType GetType(void) const { return ObjType::SpotLight; }

	void SetOrigin(const CPt& ori);
	const CPt& GetOrigin(void) const;
	void SetDirection(const CUnitVector& dir);
	const CUnitVector& GetDirection(void) const;
	void SetConcentration(double concentration);
	double GetConcentration(void) const;
	void SetDropoff(double full, double drop);
	double GetDropoffFull(void) const;
	double GetDropoffDrop(void) const;

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	// animation
	virtual void PreAnimate(void);
	virtual void PostAnimate(void);
	virtual void ResetPlayback(void);
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	virtual void RenderStart(const CMatrix& itm);
	virtual bool GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist);
	virtual bool DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
		COLOR& diffcol, COLOR& speccol);
};

_MIGRENDER_END
