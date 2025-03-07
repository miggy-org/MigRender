// object.cpp - defines the CObj base class
//

#include <stdexcept>

#include "object.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CObj
//-----------------------------------------------------------------------------

CObj::CObj(void)
	: m_diff(0), m_spec(0), m_glow(0), m_refl(0), m_refr(0, 0, 0, 1), m_index(1), m_super(false)
{
	m_flags = OBJF_ALL_RAYS | OBJF_SHADOW_CASTER;
	m_filter = TextureFilter::Bilinear;
}

CObj::~CObj(void)
{
}

void CObj::operator=(const CObj& rhs)
{
	Duplicate(rhs);
}

void CObj::Duplicate(const CObj& rhs)
{
	CBaseObj::Duplicate(rhs);

	m_diff = rhs.m_diff;
	m_spec = rhs.m_spec;
	m_refl = rhs.m_refl;
	m_refr = rhs.m_refr;
	m_glow = rhs.m_glow;
	m_index = rhs.m_index;
	m_flags = rhs.m_flags;
	m_ftm = rhs.m_ftm;
	m_bbox = rhs.m_bbox;
	m_sbox = rhs.m_sbox;

	DupTextureArray(TextureMapType::Diffuse, m_dmaps, rhs.m_dmaps);
	DupTextureArray(TextureMapType::Specular, m_smaps, rhs.m_smaps);
	DupTextureArray(TextureMapType::Refraction, m_fmaps, rhs.m_fmaps);
	DupTextureArray(TextureMapType::Glow, m_gmaps, rhs.m_gmaps);
	DupTextureArray(TextureMapType::Reflection, m_rmaps, rhs.m_rmaps);
	DupTextureArray(TextureMapType::Transparency, m_tmaps, rhs.m_tmaps);
	DupTextureArray(TextureMapType::Bump, m_bmaps, rhs.m_bmaps);
}

shared_ptr<CTexture> CObj::DupTexture(TextureMapType type, const std::shared_ptr<CTexture>& rhs)
{
	auto pnew = make_shared<CTexture>();
	*pnew = *rhs;
	return pnew;
}

void CObj::DupTextureArray(TextureMapType type, TEXTURES& lhs, const TEXTURES& rhs)
{
	for (size_t i = 0; i < rhs.size(); i++)
		lhs.push_back(DupTexture(type, rhs[i]));
}

void CObj::SetDiffuse(const COLOR& diff)
{
	m_diff = diff;
}

void CObj::SetSpecular(const COLOR& spec)
{
	m_spec = spec;
}

void CObj::SetReflection(const COLOR& refl)
{
	m_refl = refl;
}

void CObj::SetRefraction(const COLOR& refr, double index, double near, double far)
{
	m_refr = refr;
	if (index > 0)
		m_index = index;
	m_refrNear = near;
	m_refrFar = far;
}

void CObj::SetGlow(const COLOR& glow)
{
	m_glow = glow;
}

void CObj::SetObjectFlags(dword flags)
{
	m_flags = flags;
}

const COLOR& CObj::GetDiffuse(void) const
{
	return m_diff;
}

const COLOR& CObj::GetSpecular(void) const
{
	return m_spec;
}

const COLOR& CObj::GetReflection(void) const
{
	return m_refl;
}

const COLOR& CObj::GetRefraction(void) const
{
	return m_refr;
}

const COLOR& CObj::GetGlow(void) const
{
	return m_glow;
}

dword CObj::GetObjectFlags(void) const
{
	return m_flags;
}

double CObj::GetIndex(void) const
{
	return m_index;
}

double CObj::GetRefractionNear(void) const
{
	return m_refrNear;
}

double CObj::GetRefractionFar(void) const
{
	return m_refrFar;
}

bool CObj::IsShadowCaster(void) const
{
	return (m_flags & OBJF_SHADOW_CASTER ? true : false);
}

bool CObj::IsShadowCatcher(void) const
{
	return (m_flags & OBJF_SHADOW_RAY ? true : false);
}

bool CObj::IsAutoReflect(void) const
{
	return (m_flags & OBJF_REFL_RAY ? true : false);
}

bool CObj::IsAutoRefract(void) const
{
	return (m_flags & OBJF_REFR_RAY ? true : false);
}

bool CObj::IsInvisible(void) const
{
	return (m_flags & OBJF_INVISIBLE ? true : false);
}

PTEXTURES CObj::GetTextures(TextureMapType type)
{
	switch (type)
	{
	case TextureMapType::Diffuse:		return &m_dmaps;
	case TextureMapType::Specular:		return &m_smaps;
	case TextureMapType::Refraction:	return &m_fmaps;
	case TextureMapType::Glow:			return &m_gmaps;
	case TextureMapType::Reflection:	return &m_rmaps;
	case TextureMapType::Transparency:	return &m_tmaps;
	case TextureMapType::Bump:			return &m_bmaps;
	default:
		throw model_exception("Unsupported TextureMapType: " + to_string(static_cast<int>(type)));
	}
}

const TEXTURES& CObj::GetTextures(TextureMapType type) const
{
	switch (type)
	{
	case TextureMapType::Diffuse:		return m_dmaps;
	case TextureMapType::Specular:		return m_smaps;
	case TextureMapType::Refraction:	return m_fmaps;
	case TextureMapType::Glow:			return m_gmaps;
	case TextureMapType::Reflection:	return m_rmaps;
	case TextureMapType::Transparency:	return m_tmaps;
	case TextureMapType::Bump:			return m_bmaps;
	default:
		throw model_exception("Unsupported TextureMapType: " + to_string(static_cast<int>(type)));
	}
}

int CObj::AddTextureMap(TextureMapType type, const std::shared_ptr<CTexture>& pmap)
{
	if (pmap != NULL)
	{
		PTEXTURES ptxts = GetTextures(type);
		ptxts->push_back(pmap);
		return (int) ptxts->size() - 1;
	}

	return -1;
}

void CObj::RemoveMapType(TextureMapType type)
{
	GetTextures(type)->clear();
}

void CObj::RemoveAllMaps(void)
{
	RemoveMapType(TextureMapType::Diffuse);
	RemoveMapType(TextureMapType::Specular);
	RemoveMapType(TextureMapType::Refraction);
	RemoveMapType(TextureMapType::Glow);
	RemoveMapType(TextureMapType::Reflection);
	RemoveMapType(TextureMapType::Transparency);
	RemoveMapType(TextureMapType::Bump);
}

void CObj::SetBoundBox(bool use)
{
	if (use)
		m_flags |= OBJF_USE_BBOX;
	else
		m_flags &= ~OBJF_USE_BBOX;
}

bool CObj::UseBoundBox(void) const
{
	return (m_flags & OBJF_USE_BBOX ? true : false);
}

void CObj::ComputeBoundBox(CBoundBox& bbox, const CMatrix& itm) const
{
	CMatrix ftm = m_tm;
	ftm.MatrixMultiply(itm);
	ftm.ComputeInverseTranspose();
	GetBoundBox(ftm, bbox);
}

int CObj::AddColorMap(TextureMapType type, TextureMapOp op, dword flags, const string& map, UVC uvMin, UVC uvMax)
{
	if (type == TextureMapType::Diffuse || type == TextureMapType::Specular || type == TextureMapType::Refraction || type == TextureMapType::Glow)
	{
		auto pnew = make_shared<CTexture>();
		pnew->Init(map, (flags | TXTF_ENABLED), op, uvMin, uvMax);
		return CObj::AddTextureMap(type, pnew);
	}
	return -1;
}

int CObj::AddReflectionMap(dword flags, const string& map, UVC uvMin, UVC uvMax)
{
	auto pnew = make_shared<CTexture>();
	pnew->Init(map, (flags | TXTF_ENABLED), TextureMapOp::Multiply, uvMin, uvMax);
	return CObj::AddTextureMap(TextureMapType::Reflection, pnew);
}

int CObj::AddTransparencyMap(dword flags, const string& map, UVC uvMin, UVC uvMax)
{
	auto pnew = make_shared<CTexture>();
	pnew->Init(map, (flags | TXTF_ENABLED), TextureMapOp::Multiply, uvMin, uvMax);
	return CObj::AddTextureMap(TextureMapType::Transparency, pnew);
}

int CObj::AddBumpMap(dword flags, const string& map, double bscale, int btol, UVC uvMin, UVC uvMax)
{
	auto pnew = make_shared<CTexture>();
	pnew->Init(map, (flags | TXTF_ENABLED | TXTF_IS_BUMP), TextureMapOp::Multiply, uvMin, uvMax);
	pnew->SetBumpParams(bscale, btol);
	return CObj::AddTextureMap(TextureMapType::Bump, pnew);
}

bool CObj::ShouldSuperSample(void) const
{
	return m_super;
}

void CObj::LoadMap(BlockType bt, CFileBase& fobj)
{
	auto pnew = make_shared<CTexture>();
	TextureMapType type = pnew->Load(fobj);
	if (type == TextureMapType::None)
		throw fileio_exception("Invalid texture map type loaded");
	CObj::AddTextureMap(type, pnew);
}

void CObj::Load(CFileBase& fobj)
{
	BlockType bt = fobj.ReadNextBlockType();
	while (bt != BlockType::EndRange)
	{
		if (bt == BlockType::Texture || bt == BlockType::PTexture)
			LoadMap(bt, fobj);

		bt = fobj.ReadNextBlockType();
	}

	CBaseObj::Load(fobj);
}

void CObj::SaveMapType(TextureMapType type, CFileBase& fobj)
{
	PTEXTURES pmaps = GetTextures(type);
	for (const auto& iter : *pmaps)
		iter->Save(fobj, type);
}

void CObj::Save(CFileBase& fobj)
{
	SaveMapType(TextureMapType::Diffuse, fobj);
	SaveMapType(TextureMapType::Specular, fobj);
	SaveMapType(TextureMapType::Refraction, fobj);
	SaveMapType(TextureMapType::Glow, fobj);
	SaveMapType(TextureMapType::Reflection, fobj);
	SaveMapType(TextureMapType::Transparency, fobj);
	SaveMapType(TextureMapType::Bump, fobj);
	if (!fobj.WriteDataBlock(BlockType::EndRange, NULL, 0))
		throw fileio_exception("Unable to write end range block");

	CBaseObj::Save(fobj);
}

void CObj::PreAnimate(void)
{
	CBaseObj::PreAnimate();

	m_diffOrig = m_diff;
	m_specOrig = m_spec;
	m_reflOrig = m_refl;
	m_refrOrig = m_refr;
	m_glowOrig = m_glow;
}

void CObj::PostAnimate(void)
{
	CBaseObj::PostAnimate();
}

void CObj::ResetPlayback()
{
	CBaseObj::ResetPlayback();

	m_diff = m_diffOrig;
	m_spec = m_specOrig;
	m_refl = m_reflOrig;
	m_refr = m_refrOrig;
	m_glow = m_glowOrig;
}

void CObj::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	switch (animId)
	{
	case AnimType::ColorDiffuse:
		m_diff = ComputeNewAnimValue(m_diff, *((COLOR*)newValue), opType);
		break;
	case AnimType::ColorSpecular:
		m_spec = ComputeNewAnimValue(m_spec, *((COLOR*)newValue), opType);
		break;
	case AnimType::ColorReflection:
		m_refl = ComputeNewAnimValue(m_refl, *((COLOR*)newValue), opType);
		break;
	case AnimType::ColorRefraction:
		m_refr = ComputeNewAnimValue(m_refr, *((COLOR*)newValue), opType);
		break;
	case AnimType::ColorGlow:
		m_glow = ComputeNewAnimValue(m_glow, *((COLOR*)newValue), opType);
		break;
	case AnimType::IndexRefraction:
		m_index = ComputeNewAnimValue(m_index, *((double*)newValue), opType);
		break;
	default:
		CBaseObj::Animate(animId, opType, newValue);
	}
}

static void ApplyTextureOp(TextureMapOp op, dword flags, COLOR& lhs, const COLOR& rhs)
{
	if (op == TextureMapOp::Add)
		lhs += rhs;
	else if (op == TextureMapOp::Subtract)
		lhs -= rhs;
	else if (op == TextureMapOp::Blend)
	{
		double a = (flags & TXTF_INVERT_ALPHA ? rhs.a : 1 - rhs.a);
		if (a < 1)
		{
			lhs *= 1 - a;
			lhs += (rhs * a);
		}
	}
	else
		lhs *= rhs;
}

bool CObj::PostIntersectColorMap(int nthread, TextureMapType type, COLOR& col)
{
	PTEXTURES pmaps = GetTextures(type);
	if (pmaps->size() == 0)
		return false;

	bool ret = false;
	for (const auto& ptxt : *pmaps)
	{
		if (ptxt && ptxt->IsEnabled())
		{
			UVC final;
			if (ComputeTexelCoord(nthread, ptxt.get(), final))
			{
				COLOR mapcol;
				ptxt->GetTexel(final, m_filter, mapcol);
				ApplyTextureOp(ptxt->GetOperation(), m_flags, col, mapcol);

				ret = true;
			}
		}
	}

	return ret;
}

bool CObj::PostIntersectReflectMap(int nthread, const INTERSECTION& intr, COLOR& col)
{
	if (m_rmaps.size() == 0)
		return false;

	bool ret = false;
	for (const auto& ptxt : m_rmaps)
	{
		//const CTexture* ptxt = (CTexture*) m_rmaps[n].get();
		if (ptxt->IsEnabled())
		{
			UVC final;

			double f2ndoti = 2*intr.norm.DotProduct(intr.gray.dir);
			CUnitVector tmp = intr.gray.dir - intr.norm*f2ndoti;
			tmp.Normalize();

			tmp.x = -tmp.x;
			tmp.y = 0;
			if (tmp.Normalize() > 0)
			{
				final.u = atan2(tmp.z, tmp.x);
				if (final.u >= 0)
					final.u = final.u / (2*M_PI);
				else
					final.u = (M_PI + final.u) / (2*M_PI) + 0.5;
			}
			else
				final.u = 0.5;
			final.v = (1 + intr.norm.y) / 2.0;

			COLOR mapcol;
			ptxt->GetTexel(final, m_filter, mapcol);
			ApplyTextureOp(ptxt->GetOperation(), m_flags, col, mapcol);

			ret = true;
		}
	}

	return ret;
}

bool CObj::PostIntersectTransparencyMap(int nthread, double& alpha)
{
	alpha = 1;
	if (m_tmaps.size() == 0)
		return false;

	bool ret = false;
	//for (int n = 0; n < (int) m_tmaps.size(); n++)
	for (const auto& ptxt : m_tmaps)
	{
		//const CTexture* ptxt = (CTexture*) m_tmaps[n].get();
		if (ptxt && ptxt->IsEnabled())
		{
			UVC final;
			if (ComputeTexelCoord(nthread, ptxt.get(), final))
			{
				double tmp = 1;
				if (ptxt->GetAlphaTexel(final, m_filter, tmp))
				{
					alpha *= tmp;
					ret = true;
				}
			}
		}
	}

	return ret;
}

// transform the offset vector to the normal's space
bool CObj::ComputeBumpMapToNormTM(int nthread, const CUnitVector& norm, CMatrix& tm)
{
	double rotx = 0, roty = 0, rotz = 0;
	CUnitVector tmp;

	tmp.SetPoint(0, norm.y, (norm.z > 0 ? norm.z : -norm.z));
	if (tmp.Normalize() > 0)
		rotx = -atan2(tmp.y, tmp.z);

	tmp.SetPoint(norm.x, 0, norm.z);
	if (tmp.Normalize() > 0)
	{
		roty = atan2(tmp.x, tmp.z);
		if (roty < 0)
			roty += 2*M_PI;
	}

	// TODO: still need to figure out how to compute the z rotation
	rotz = 0;

	tm.RotateX(rotx, true);
	tm.RotateY(roty, true);
	tm.RotateZ(rotz, true);
	return true;
}

bool CObj::PostIntersectBumpMap(int nthread, INTERSECTION& intr)
{
	if (m_bmaps.size() == 0)
		return false;

	CMatrix tm;
	ComputeBumpMapToNormTM(nthread, intr.norm, tm);

	bool ret = false;
	//for (int n = 0; n < (int) m_bmaps.size(); n++)
	for (const auto& ptxt : m_bmaps)
	{
		//const CTexture* ptxt = (CTexture*) m_bmaps[n].get();
		if (ptxt->IsEnabled())
		{
			CPt offset;

			// get the original offset vector (in the original image UV space)
			if (ptxt->IsAutoBump())
			{
				CPt lhitpt = intr.lray.ori + intr.lray.dir*intr.llen;
				ptxt->GetAutoBumpVector(lhitpt, offset);
			}
			else
			{
				UVC final;
				ComputeTexelCoord(nthread, ptxt.get(), final);
				ptxt->GetBumpVector(final, m_filter, offset);
			}

			// transform the offset vector to the normal's space
			tm.TransformPoint(offset, 0, MatrixType::CTM);

			// finally, perturb the normal
			intr.norm = intr.norm + offset;
			ret = true;
		}
	}

	if (ret)
		intr.norm.Normalize();
	return ret;
}

void CObj::RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images)
{
	if (IsInvisible())
		return;
	m_ftm = m_tm;
	m_ftm.MatrixMultiply(itm);
	m_ftm.ComputeInverseTranspose();

	// compute the bounding box (global coords)
	if (UseBoundBox())
		GetBoundBox(m_ftm, m_bbox);

	// pre-render texture maps
	TextureMapType types[] = {
		TextureMapType::Diffuse,
		TextureMapType::Specular,
		TextureMapType::Refraction,
		TextureMapType::Glow,
		TextureMapType::Reflection,
		TextureMapType::Transparency,
		TextureMapType::Bump };
	for (int i = 0; i < sizeof(types) / sizeof(TextureMapType); i++)
	{
		const PTEXTURES pmaps = GetTextures(types[i]);
		for (const auto& iter : *pmaps)
		{
			if (iter->IsEnabled())
				iter->PreRender(images.GetImage(iter->GetMapName()));
		}
	}

	// determine if this object could profit from super sampling
	m_super = false;
	if (m_flags & OBJF_ALL_RAYS)
		m_super = true;
	else if (m_rmaps.size() > 0)
	{
		for (const auto& iter : m_rmaps)
		{
			if (iter->IsEnabled())
				m_super = true;
		}
	}
	else if (m_bmaps.size() > 0)
	{
		for (const auto& iter : m_bmaps)
		{
			if (iter->IsEnabled() && iter->IsAutoBump())
				m_super = true;
		}
	}
}

void CObj::RenderFinish(void)
{
	TextureMapType types[] = {
		TextureMapType::Diffuse,
		TextureMapType::Specular,
		TextureMapType::Refraction,
		TextureMapType::Glow,
		TextureMapType::Reflection,
		TextureMapType::Transparency,
		TextureMapType::Bump };
	for (int i = 0; i < sizeof(types) / sizeof(TextureMapType); i++)
	{
		const PTEXTURES pmaps = GetTextures(types[i]);
		for (const auto& iter : *pmaps)
		{
			if (iter->IsEnabled())
				iter->PostRender();
		}
	}
}

bool CObj::IntersectShadowRay(int nthread, const CRay& ray, double max)
{
	INTERSECTION intr;
	if (IsShadowCaster() && IntersectRay(nthread, ray, intr))
	{
		if (max == -1 || intr.glen < max)
			return true;
	}

	return false;
}

bool CObj::PostIntersect(int nthread, INTERSECTION& intr)
{
	// at this point we assume that the normal has been set up by a derived class
	PostIntersectBumpMap(nthread, intr);
	m_ftm.TransformPoint(intr.norm, 0, MatrixType::CTM);
	intr.norm.Normalize();

	// color mapping
	intr.diff = GetDiffuse();
	PostIntersectColorMap(nthread, TextureMapType::Diffuse, intr.diff);
	intr.spec = GetSpecular();
	PostIntersectColorMap(nthread, TextureMapType::Specular, intr.spec);
	intr.refr = GetRefraction();
	PostIntersectColorMap(nthread, TextureMapType::Refraction, intr.refr);
	intr.glow = GetGlow();
	PostIntersectColorMap(nthread, TextureMapType::Glow, intr.glow);

	// transparency mapping
	double transparency;
	if (PostIntersectTransparencyMap(nthread, transparency))
		intr.refr.a *= transparency;

	// reflection mapping
	intr.refl = GetReflection();
	if (!PostIntersectReflectMap(nthread, intr, intr.refl))
		intr.refl.Init(0);

	// the refraction color scales down other colors
	if (intr.refr.a < 1)
	{
		intr.diff *= intr.refr.a;
		intr.spec *= intr.refr.a;
		intr.glow *= intr.refr.a;
		intr.refl *= intr.refr.a;
	}
	intr.refr *= (1 - intr.refr.a);

	return true;
}
