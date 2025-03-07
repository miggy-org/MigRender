// lights.cpp - defines lighting
//

#include "lights.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CLight
//-----------------------------------------------------------------------------

CLight::CLight(void)
	: m_col(0), m_hlit(-1), m_flags(LITF_SHADOW), m_sscale(1), m_hlitOrig(-1), m_sscaleOrig(1)
{
}

CLight::~CLight(void)
{
}

void CLight::SetColor(const COLOR& col)
{
	m_col = col;
}

const COLOR& CLight::GetColor(void) const
{
	return m_col;
}

void CLight::SetHighlight(double hlit)
{
	m_hlit = hlit;
}

double CLight::GetHighlight(void) const
{
	return m_hlit;
}

void CLight::SetShadowCaster(bool shadow, bool soft, double sscale)
{
	m_flags = LITF_NONE;
	if (shadow)
	{
		m_flags |= LITF_SHADOW;
		if (soft)
			m_flags |= LITF_SOFT_SHADOW;
	}
	m_sscale = sscale;
}

bool CLight::IsShadowCaster(void) const
{
	return (m_flags & LITF_SHADOW ? true : false);
}

bool CLight::IsSoftShadow(void) const
{
	return (m_flags & LITF_SOFT_SHADOW ? true : false);
}

double CLight::GetSoftScale(void) const
{
	return m_sscale;
}

void CLight::Load(CFileBase& fobj)
{
	CBaseObj::Load(fobj);
}

void CLight::Save(CFileBase& fobj)
{
	CBaseObj::Save(fobj);
}

void CLight::PreAnimate(void)
{
	CBaseObj::PreAnimate();

	m_colOrig = m_col;
	m_hlitOrig = m_hlit;
	m_sscaleOrig = m_sscale;
}

void CLight::PostAnimate(void)
{
	CBaseObj::PostAnimate();
}

void CLight::ResetPlayback(void)
{
	CBaseObj::ResetPlayback();

	m_col = m_colOrig;
	m_hlit = m_hlitOrig;
	m_sscale = m_sscaleOrig;
}

void CLight::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	switch (animId)
	{
	case AnimType::LightHLit: m_hlit = ComputeNewAnimValue(m_hlit, *((const double*)newValue), opType); break;
	case AnimType::LightSScale: m_sscale = ComputeNewAnimValue(m_sscale, *((const double*)newValue), opType); break;
	case AnimType::LightColor: m_col = ComputeNewAnimValue(m_col, *((COLOR*)newValue), opType); break;
	default: CBaseObj::Animate(animId, opType, newValue); break;
	}
}

void CLight::RenderStart(const CMatrix& itm)
{
	m_ftm = m_tm;
	m_ftm.MatrixMultiply(itm);
	m_ftm.ComputeInverseTranspose();
}

void CLight::RenderFinish(void)
{
}

//-----------------------------------------------------------------------------
// CDirLight
//-----------------------------------------------------------------------------

CDirLight::CDirLight(void)
	: m_dir(0, 0, 1)
{
}

void CDirLight::SetDirection(const CUnitVector& dir)
{
	m_dir = dir;
}

const CUnitVector& CDirLight::GetDirection(void) const
{
	return m_dir;
}

bool CDirLight::IsSoftShadow(void) const
{
	return false;
}

void CDirLight::Load(CFileBase& fobj)
{
	FILE_DIR_LIGHT fdl;
	if (!fobj.ReadNextBlock((byte*)&fdl, sizeof(FILE_DIR_LIGHT)))
		throw fileio_exception("Unable to read FILE_DIR_LIGHT");
	m_col = fdl.base.col;
	m_hlit = fdl.base.hlit;
	m_flags = fdl.base.flags;
	m_sscale = fdl.base.sscale;
	m_tm = fdl.base.tm;
	m_dir = fdl.dir;
	CLight::Load(fobj);
}

void CDirLight::Save(CFileBase& fobj)
{
	FILE_DIR_LIGHT fdl;
	memset(&fdl, 0, sizeof(FILE_DIR_LIGHT));
	fdl.base.col = m_col;
	fdl.base.hlit = m_hlit;
	fdl.base.sscale = m_sscale;
	fdl.base.flags = m_flags;
	fdl.base.tm = m_tm;

	fdl.dir = m_dir;
	if (!fobj.WriteDataBlock(BlockType::DirLight, (byte*)&fdl, sizeof(FILE_DIR_LIGHT)))
		throw fileio_exception("Unable to write FILE_DIR_LIGHT");
}

void CDirLight::RenderStart(const CMatrix& itm)
{
	CLight::RenderStart(itm);
	m_fdir = m_dir;
	m_ftm.TransformPoint(m_fdir, 0, MatrixType::CTM);
}

bool CDirLight::GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist)
{
	dir = m_fdir;
	dir.Invert();
	dist = -1;
	return true;
}

bool CDirLight::DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
						   COLOR& diffcol, COLOR& speccol)
{
	bool ret = false;
	diffcol.Init();
	speccol.Init();

	double scale = -m_fdir.DotProduct(normal);
	if (scale > 0.01)
	{
		diffcol = m_col;
		diffcol *= scale;
		ret = true;
	}

	if (m_hlit > 0)
	{
		CUnitVector halfvec = eye - m_fdir;
		halfvec.Normalize();
		scale = halfvec.DotProduct(normal);
		if (scale > 0.01)
		{
			scale = pow(scale, m_hlit);
			speccol = m_col;
			speccol *= scale;
			ret = true;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// CPtLight
//-----------------------------------------------------------------------------

CPtLight::CPtLight(void)
	: m_ori(0, 0, 0), m_fulldist(0), m_dropdist(0), m_droplen(0), m_dropdistOrig(0), m_fulldistOrig(0)
{
}

void CPtLight::SetOrigin(const CPt& ori)
{
	m_ori = ori;
}

const CPt& CPtLight::GetOrigin(void) const
{
	return m_ori;
}

void CPtLight::SetDropoff(double full, double drop)
{
	if (full <= drop)
	{
		m_fulldist = full;
		m_dropdist = drop;
		m_droplen = drop - full;
	}
}

double CPtLight::GetDropoffFull(void) const
{
	return m_fulldist;
}

double CPtLight::GetDropoffDrop(void) const
{
	return m_dropdist;
}

void CPtLight::Load(CFileBase& fobj)
{
	FILE_PT_LIGHT fpl;
	if (!fobj.ReadNextBlock((byte*) &fpl, sizeof(FILE_PT_LIGHT)))
		throw fileio_exception("Unable to read FILE_PT_LIGHT");
	m_col = fpl.base.col;
	m_hlit = fpl.base.hlit;
	m_flags = fpl.base.flags;
	m_sscale = fpl.base.sscale;
	m_tm = fpl.base.tm;
	m_ori = fpl.ori;
	m_fulldist = fpl.fulldist;
	m_dropdist = fpl.dropdist;
	m_droplen = fpl.droplen;
}

void CPtLight::Save(CFileBase& fobj)
{
	FILE_PT_LIGHT fpl;
	memset(&fpl, 0, sizeof(FILE_PT_LIGHT));
	fpl.base.col = m_col;
	fpl.base.hlit = m_hlit;
	fpl.base.flags = m_flags;
	fpl.base.sscale = m_sscale;
	fpl.base.tm = m_tm;

	fpl.ori = m_ori;
	fpl.fulldist = m_fulldist;
	fpl.dropdist = m_dropdist;
	fpl.droplen = m_droplen;
	if (!fobj.WriteDataBlock(BlockType::PtLight, (byte*) &fpl, sizeof(FILE_PT_LIGHT)))
		throw fileio_exception("Unable to write FILE_PT_LIGHT");
}

void CPtLight::PreAnimate(void)
{
	CLight::PreAnimate();

	m_fulldistOrig = m_fulldist;
	m_dropdistOrig = m_dropdist;
}

void CPtLight::PostAnimate(void)
{
	CLight::PostAnimate();
}

void CPtLight::ResetPlayback(void)
{
	CLight::ResetPlayback();

	m_fulldist = m_fulldistOrig;
	m_dropdist = m_dropdistOrig;
}

void CPtLight::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	double value = *((const double*)newValue);

	switch (animId)
	{
	case AnimType::LightFullDistance: m_fulldist = ComputeNewAnimValue(m_fulldist, value, opType); break;
	case AnimType::LightDropDistance: m_dropdist = ComputeNewAnimValue(m_dropdist, value, opType); break;
	default: CLight::Animate(animId, opType, newValue); break;
	}
}

void CPtLight::RenderStart(const CMatrix& itm)
{
	CLight::RenderStart(itm);
	m_fori = m_ori;
	m_ftm.TransformPoint(m_fori, 1, MatrixType::CTM);
}

bool CPtLight::GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist)
{
	dir = m_fori - ori;
	dist = dir.GetLength();
	dir.Normalize();
	return (dist > 0);
}

bool CPtLight::DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
						  COLOR& diffcol, COLOR& speccol)
{
	bool ret = false;
	diffcol.Init();
	speccol.Init();

	CUnitVector dir = m_fori - intpt;
	double scale = 1;
	if (m_dropdist > 0)
	{
		double len = dir.GetLength();
		if (len > m_dropdist)
			scale = 0;
		else if (len > m_fulldist)
			scale *= 1 - ((len - m_fulldist) / m_droplen);
	}
	if (scale > 0.01)
	{
		dir.Normalize();
		scale *= dir.DotProduct(normal);
		if (scale > 0)
		{
			diffcol = m_col;
			diffcol *= scale;
			ret = true;
		}
	}

	if (m_hlit > 0)
	{
		CUnitVector halfvec = eye + dir;
		halfvec.Normalize();
		scale = halfvec.DotProduct(normal);
		if (scale > 0.01)
		{
			scale = pow(scale, m_hlit);
			speccol = m_col;
			speccol *= scale;
			ret = true;
		}
	}

	return ret;
}

//-----------------------------------------------------------------------------
// CSpotLight
//-----------------------------------------------------------------------------

CSpotLight::CSpotLight(void)
	: m_ori(0, 0, 0), m_dir(0, 0, 1), m_concentration(1), m_fulldist(0), m_dropdist(0), m_droplen(0), m_dropdistOrig(0), m_fulldistOrig(0)
{
}

void CSpotLight::SetOrigin(const CPt& ori)
{
	m_ori = ori;
}

const CPt& CSpotLight::GetOrigin(void) const
{
	return m_ori;
}

void CSpotLight::SetDirection(const CUnitVector& dir)
{
	m_dir = dir;
}

const CUnitVector& CSpotLight::GetDirection(void) const
{
	return m_dir;
}

void CSpotLight::SetConcentration(double concentration)
{
	m_concentration = concentration;
}

double CSpotLight::GetConcentration(void) const
{
	return m_concentration;
}

void CSpotLight::SetDropoff(double full, double drop)
{
	if (full <= drop)
	{
		m_fulldist = full;
		m_dropdist = drop;
		m_droplen = drop - full;
	}
}

double CSpotLight::GetDropoffFull(void) const
{
	return m_fulldist;
}

double CSpotLight::GetDropoffDrop(void) const
{
	return m_dropdist;
}

void CSpotLight::Load(CFileBase& fobj)
{
	FILE_SPOT_LIGHT fsl;
	if (!fobj.ReadNextBlock((byte*) &fsl, sizeof(FILE_SPOT_LIGHT)))
		throw fileio_exception("Unable to read FILE_SPOT_LIGHT");
	m_col = fsl.base.col;
	m_hlit = fsl.base.hlit;
	m_flags = fsl.base.flags;
	m_sscale = fsl.base.sscale;
	m_tm = fsl.base.tm;
	m_ori = fsl.ori;
	m_dir = fsl.dir;
	m_fulldist = fsl.fulldist;
	m_dropdist = fsl.dropdist;
	m_droplen = fsl.droplen;
	m_concentration = fsl.concentration;
}

void CSpotLight::Save(CFileBase& fobj)
{
	FILE_SPOT_LIGHT fsl;
	memset(&fsl, 0, sizeof(FILE_SPOT_LIGHT));
	fsl.base.col = m_col;
	fsl.base.hlit = m_hlit;
	fsl.base.flags = m_flags;
	fsl.base.sscale = m_sscale;
	fsl.base.tm = m_tm;

	fsl.ori = m_ori;
	fsl.dir = m_dir;
	fsl.fulldist = m_fulldist;
	fsl.dropdist = m_dropdist;
	fsl.droplen = m_droplen;
	fsl.concentration = m_concentration;
	if (!fobj.WriteDataBlock(BlockType::SpotLight, (byte*) &fsl, sizeof(FILE_SPOT_LIGHT)))
		throw fileio_exception("Unable to write FILE_SPOT_LIGHT");
}

void CSpotLight::PreAnimate(void)
{
	CLight::PreAnimate();

	m_fulldistOrig = m_fulldist;
	m_dropdistOrig = m_dropdist;
}

void CSpotLight::PostAnimate(void)
{
	CLight::PostAnimate();
}

void CSpotLight::ResetPlayback(void)
{
	CLight::ResetPlayback();

	m_fulldist = m_fulldistOrig;
	m_dropdist = m_dropdistOrig;
}

void CSpotLight::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	double value = *((const double*)newValue);

	switch (animId)
	{
	case AnimType::LightFullDistance: m_fulldist = ComputeNewAnimValue(m_fulldist, value, opType); break;
	case AnimType::LightDropDistance: m_dropdist = ComputeNewAnimValue(m_dropdist, value, opType); break;
	default: CLight::Animate(animId, opType, newValue); break;
	}
}

void CSpotLight::RenderStart(const CMatrix& itm)
{
	CLight::RenderStart(itm);
	m_fori = m_ori;
	m_fdir = m_dir;
	m_ftm.TransformPoint(m_fori, 1, MatrixType::CTM);
	m_ftm.TransformPoint(m_fdir, 0, MatrixType::CTM);
}

bool CSpotLight::GetDirectionToLight(const CPt& ori, CUnitVector& dir, double& dist)
{
	dir = m_fori - ori;
	dist = dir.GetLength();
	dir.Normalize();
	return (dist > 0);
}

bool CSpotLight::DoLighting(const CPt& intpt, const CUnitVector& normal, const CUnitVector& eye,
							COLOR& diffcol, COLOR& speccol)
{
	double hlitscale = 0;
	bool ret = false;
	diffcol.Init();
	speccol.Init();

	CUnitVector dir = m_fori - intpt;
	double scale = 1;
	if (scale > 0.01)
	{
		CUnitVector udir = dir;
		udir.Normalize();
		double value = -udir.DotProduct(m_fdir);
		if (value > 0)
			scale = pow(value, m_concentration);
		else
			scale = 0;
		hlitscale = scale;
	}
	if (scale > 0.01 && m_dropdist > 0)
	{
		double len = dir.GetLength();
		if (len > m_dropdist)
			scale = 0;
		else if (len > m_fulldist)
			scale *= 1 - ((len - m_fulldist) / m_droplen);
	}
	if (scale > 0.01)
	{
		dir.Normalize();
		scale *= dir.DotProduct(normal);
		if (scale > 0)
		{
			diffcol = m_col;
			diffcol *= scale;
			ret = true;
		}
	}

	if (m_hlit > 0 && hlitscale > 0)
	{
		CUnitVector halfvec = eye + dir;
		halfvec.Normalize();
		scale = halfvec.DotProduct(normal);
		if (scale > 0.01)
		{
			scale = pow(scale, m_hlit);
			scale *= hlitscale;
			if (scale > 0.01)
			{
				speccol = m_col;
				speccol *= scale;
				ret = true;
			}
		}
	}

	return ret;
}
