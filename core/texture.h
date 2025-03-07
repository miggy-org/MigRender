// texture.h - defines texture mapping routines
//

#pragma once

#include <vector>

#include "defines.h"
#include "templates.h"
#include "image.h"
#include "vector.h"
#include "fileio.h"

// texturing flags
#define TXTF_NONE				0x00000000
#define TXTF_ENABLED			0x00000001
#define TXTF_RGB				0x00000002
#define TXTF_ALPHA				0x00000004
#define TXTF_INVERT				0x00000008
#define TXTF_IS_BUMP			0x00000010
#define TXTF_INVERT_ALPHA		0x00000020

#define TXTF_POLY_INDC			0x00010000

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CTexture - texture mapping base class
//-----------------------------------------------------------------------------

class CTexture
{
protected:
	TextureMapOp m_op;
	std::string m_map;
	dword m_flags;

	// used to constrain map
	UVC m_uvMin, m_uvMax;

	// used for bump mapping
	double m_bscale;
	int m_btol;

	// rendering variables (thread safe)
	CImageBuffer* m_pmap;
	CImageBuffer* m_pbumpmap;
	bool m_constrainUVC;
	UVC m_constrainRange;

protected:
	bool CreateBumpMap(void);
	UVC GetConstrainedUVCFast(const UVC& uv) const;

public:
	CTexture(void);
	virtual ~CTexture(void);

	virtual void Init(const std::string& map, dword flags, TextureMapOp op = TextureMapOp::Multiply, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	virtual void SetBumpParams(double bscale, int btol);
	virtual void Delete(void);

	// file I/O
	virtual TextureMapType Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj, TextureMapType type);

	void PreRender(CImageBuffer* pmap);
	void PostRender(void);

	UVC GetConstrainedUVC(const UVC& uv) const;
	bool GetTexel(const UVC& uv, TextureFilter filter, COLOR& col) const;
	bool GetAlphaTexel(const UVC& uv, TextureFilter filter, double& alpha) const;
	bool GetBumpVector(const UVC& uv, TextureFilter filter, CPt& offset) const;
	bool GetAutoBumpVector(const CPt& hitpt, CPt& offset) const;

	void Enable(bool enable)
		{ (enable ? (m_flags |= TXTF_ENABLED) : (m_flags &= ~TXTF_ENABLED)); }
	bool IsEnabled(void) const
		{ return (m_flags & TXTF_ENABLED); }
	bool IsAutoBump(void) const
		{ return (m_flags == (TXTF_ENABLED|TXTF_IS_BUMP) && m_pmap == NULL); }
	dword GetFlags(void) const
		{ return m_flags; }
	const std::string& GetMapName(void) const
		{ return m_map; }
	TextureMapOp GetOperation(void) const
		{ return m_op; }
	void GetUVConstraint(UVC& uvMin, UVC& uvMax) const
		{ uvMin = m_uvMin; uvMax = m_uvMax; }
};
typedef std::vector<std::shared_ptr<CTexture>> TEXTURES;
typedef TEXTURES* PTEXTURES;

//-----------------------------------------------------------------------------
// CPolyTexture - defines a polygon mesh texture map
//-----------------------------------------------------------------------------

class CPolyTexture : public CTexture
{
protected:
	CLoadArray<UVC> m_coords;

public:
	CPolyTexture(void);

	virtual void Delete(void);

	// file I/O
	virtual TextureMapType Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj, TextureMapType type);

	bool IsValid(void) const
		{ return (m_coords.CountLoaded() > 0); }
	bool IsInd(void) const
		{ return (m_flags & TXTF_POLY_INDC ? true : false); }
	const UVC& GetCoord(int index) const
		{ return m_coords[index]; }

	bool LoadCoords(const UVC* uvcs, int num);
	bool LoadCoord(const UVC& uvc);
	bool LoadComplete(void);

	void ChangeCoord(int index, const UVC& uvc);
	void DeleteCoords(void);

	int Count(void);
};

_MIGRENDER_END
