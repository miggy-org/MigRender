// object.h - defines the object base class
//

#pragma once

#include <vector>
#include <string>
#include <map>

#include "vector.h"
#include "ray.h"
#include "matrix.h"
#include "bbox.h"
#include "baseobject.h"
#include "camera.h"
#include "image.h"
#include "texture.h"

// object flags
#define OBJF_NONE				0x00000000
#define OBJF_SHADOW_RAY			0x00000001
#define OBJF_REFL_RAY			0x00000002
#define OBJF_REFR_RAY			0x00000004
#define OBJF_SHADOW_CASTER		0x00000008
#define OBJF_USE_BBOX			0x00000010
#define OBJF_INVISIBLE			0x00000020
#define OBJF_ALL_RAYS			(OBJF_SHADOW_RAY|OBJF_REFL_RAY|OBJF_REFR_RAY)

_MIGRENDER_BEGIN

struct INTERSECTION
{
	// used during intersection testing
	CRay lray;
	double glen;
	double llen;

	// used after intersection testing
	CUnitVector norm;
	COLOR diff;
	COLOR spec;
	COLOR refl;
	COLOR refr;
	COLOR glow;

	// set up by CModel
	CRay gray;
	CPt hit;
	CUnitVector eye;
};

//-----------------------------------------------------------------------------
// CObj - object base class
//
//  This class is abstract - derived implementations are what will actually
//  appear in the models.
//-----------------------------------------------------------------------------

class CObj : public CBaseObj
{
protected:
	COLOR m_diff;
	COLOR m_spec;
	COLOR m_refl;
	COLOR m_refr;
	COLOR m_glow;
	double m_index;
	double m_refrNear;
	double m_refrFar;
	dword m_flags;

	CMatrix m_ftm;

	TEXTURES m_dmaps;
	TEXTURES m_smaps;
	TEXTURES m_fmaps;
	TEXTURES m_gmaps;
	TEXTURES m_rmaps;
	TEXTURES m_tmaps;
	TEXTURES m_bmaps;
	TextureFilter m_filter;

	CBoundBox m_bbox;
	CScreenBox m_sbox;

	// temporary rendering variables
	bool m_super;

	// temporary animation variables
	COLOR m_diffOrig;
	COLOR m_specOrig;
	COLOR m_reflOrig;
	COLOR m_refrOrig;
	COLOR m_glowOrig;
	double m_indexOrig;

protected:
	void Duplicate(const CObj& rhs);

	PTEXTURES GetTextures(TextureMapType type);
	int AddTextureMap(TextureMapType type, const std::shared_ptr<CTexture>& pmap);
	void RemoveMapType(TextureMapType type);
	void RemoveAllMaps(void);

	virtual std::shared_ptr<CTexture> DupTexture(TextureMapType type, const std::shared_ptr<CTexture>& rhs);
	virtual void DupTextureArray(TextureMapType type, TEXTURES& lhs, const TEXTURES& rhs);

	// file I/O
	virtual void LoadMap(BlockType bt, CFileBase& fobj);
	virtual void SaveMapType(TextureMapType type, CFileBase& fobj);

	virtual void GetBoundBox(const CMatrix& tm, CBoundBox& bbox) const = 0;

	virtual bool ComputeTexelCoord(int nthread, const CTexture* ptxt, UVC& final) = 0;
	virtual bool ComputeBumpMapToNormTM(int nthread, const CUnitVector& norm, CMatrix& tm);
	virtual bool PostIntersectColorMap(int nthread, TextureMapType type, COLOR& col);
	virtual bool PostIntersectReflectMap(int nthread, const INTERSECTION& intr, COLOR& col);
	virtual bool PostIntersectTransparencyMap(int nthread, double& alpha);
	virtual bool PostIntersectBumpMap(int nthread, INTERSECTION& intr);

public:
	CObj(void);
	virtual ~CObj(void);

	void operator=(const CObj& rhs);

	void SetDiffuse(const COLOR& diff);
	void SetSpecular(const COLOR& spec);
	void SetReflection(const COLOR& refl);
	void SetRefraction(const COLOR& refr, double index = 1.0, double near = -1, double far = -1);
	void SetGlow(const COLOR& glow);
	void SetObjectFlags(dword flags);

	const COLOR& GetDiffuse(void) const;
	const COLOR& GetSpecular(void) const;
	const COLOR& GetReflection(void) const;
	const COLOR& GetRefraction(void) const;
	const COLOR& GetGlow(void) const;
	dword GetObjectFlags(void) const;
	double GetIndex(void) const;
	double GetRefractionNear(void) const;
	double GetRefractionFar(void) const;
	bool IsShadowCaster(void) const;
	bool IsShadowCatcher(void) const;
	bool IsAutoReflect(void) const;
	bool IsAutoRefract(void) const;
	bool IsInvisible(void) const;

	void SetBoundBox(bool use);
	bool UseBoundBox(void) const;
	void ComputeBoundBox(CBoundBox& bbox, const CMatrix& itm) const;

	int AddColorMap(TextureMapType type, TextureMapOp op, dword flags, const std::string& map, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	int AddReflectionMap(dword flags, const std::string& map, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	int AddTransparencyMap(dword flags, const std::string& map, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	int AddBumpMap(dword flags, const std::string& map, double bscale, int btol, UVC uvMin = UVMIN, UVC uvMax = UVMAX);

	const TEXTURES& GetTextures(TextureMapType type) const;

	bool ShouldSuperSample(void) const;

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	// animation
	virtual void PreAnimate(void);
	virtual void PostAnimate(void);
	virtual void ResetPlayback(void);
	virtual void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	virtual void RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images);
	virtual void RenderFinish(void);

	virtual bool IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr) = 0;
	virtual bool IntersectShadowRay(int nthread, const CRay& ray, double max);
	virtual bool PostIntersect(int nthread, INTERSECTION& intr);
};

_MIGRENDER_END
