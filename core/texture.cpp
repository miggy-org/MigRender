// texture.cpp - defines texture mapping
//

#include "texture.h"
#include "polygon.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CTexture
//-----------------------------------------------------------------------------

CTexture::CTexture(void)
	: m_flags(TXTF_NONE), m_op(TextureMapOp::Multiply), m_bscale(1), m_btol(0), m_pmap(NULL), m_pbumpmap(NULL), m_constrainUVC(false)
{
}

CTexture::~CTexture(void)
{
	Delete();
}

void CTexture::Init(const string& map, dword flags, TextureMapOp op, UVC uvMin, UVC uvMax)
{
	m_map = map;
	m_flags = flags;
	m_op = op;
	m_uvMin = uvMin;
	m_uvMax = uvMax;
}

void CTexture::SetBumpParams(double bscale, int btol)
{
	m_bscale = bscale;
	m_btol = btol;
}

void CTexture::Delete(void)
{
	m_map.erase();
	Enable(false);
}

TextureMapType CTexture::Load(CFileBase& fobj)
{
	FILE_TEXTURE ft;
	if (!fobj.ReadNextBlock((byte*) &ft, sizeof(FILE_TEXTURE)))
		throw fileio_exception("Unable to read texture");
	m_op = ft.op;
	m_map = ft.map;
	m_uvMin = ft.uvMin;
	m_uvMax = ft.uvMax;
	m_flags = ft.flags;
	m_bscale = ft.bscale;
	m_btol = ft.btol;
	return ft.type;
}

void CTexture::Save(CFileBase& fobj, TextureMapType type)
{
	FILE_TEXTURE ft;
	ft.type = type;
	ft.op = m_op;
	if (sizeof(ft.map) <= m_map.length())
		throw fileio_exception("Texture map path length too long: " + m_map);
	strncpy(ft.map, m_map.c_str(), sizeof(ft.map));
	ft.flags = m_flags;
	ft.uvMin = m_uvMin;
	ft.uvMax = m_uvMax;
	ft.bscale = m_bscale;
	ft.btol = m_btol;
	if (!fobj.WriteDataBlock(BlockType::Texture, (byte*) &ft, sizeof(FILE_TEXTURE)))
		throw fileio_exception("Unable to write texture");
}

static int GetBumpHeight(const CImageBuffer* pmap, int x, int y, dword flags)
{
	byte data[4];
	pmap->GetPixel(x, y, ImageFormat::RGBA, data);

	int ret;
	if (flags & TXTF_ALPHA)
		ret = data[3];
	else if (pmap->GetFormat() == ImageFormat::GreyScale)
		ret = data[0];
	else
		ret = (int) ((data[0] + data[1] + data[2] + 1) / 3);
	return (flags & TXTF_INVERT ? 255 - ret : ret);
}

bool CTexture::CreateBumpMap(void)
{
	if (m_pmap != NULL)
	{
		ImageFormat fmt = m_pmap->GetFormat();
		if (!(m_flags & TXTF_ALPHA) || fmt == ImageFormat::RGBA || fmt == ImageFormat::BGRA)
		{
			int w, h;
			m_pmap->GetSize(w, h);
			m_pbumpmap = new CImageBuffer;
			m_pbumpmap->Init(w, h, ImageFormat::Bump);

			// TODO: this almost certainly could be more efficient
			vector<byte> line(2 * w);
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					int cur = GetBumpHeight(m_pmap, x, y, m_flags);
					int lasth = (x > 0 ? GetBumpHeight(m_pmap, x-1, y, m_flags) : cur);
					int nexth = (x < w-1 ? GetBumpHeight(m_pmap, x+1, y, m_flags) : cur);
					int lastv = (y > 0 ? GetBumpHeight(m_pmap, x, y-1, m_flags) : cur);
					int nextv = (y < h-1 ? GetBumpHeight(m_pmap, x, y+1, m_flags) : cur);

					if (m_btol > 0)
					{
						if (abs(lasth - cur) > m_btol)
							lasth = cur;
						if (abs(nexth - cur) > m_btol)
							nexth = cur;
						if (abs(lastv - cur) > m_btol)
							lastv = cur;
						if (abs(nextv - cur) > m_btol)
							nextv = cur;
					}

					int dh = -((nexth - cur) - (lasth - cur));
					int dv =  ((nextv - cur) - (lastv - cur));

					line[2*x] = (byte) (127 + dh/2);
					line[2*x+1] = (byte) (127 + dv/2);
				}

				m_pbumpmap->WriteLine(y, line.data(), ImageFormat::Bump);
			}
		}
	}

	return (m_pbumpmap != NULL);
}

void CTexture::PreRender(CImageBuffer* pmap)
{
	m_pmap = pmap;
	if (m_pmap != NULL)
	{
		if (m_flags & TXTF_IS_BUMP)
		{
			if (!CreateBumpMap())
				Enable(false);
		}
	}
	else if (!(m_flags & TXTF_IS_BUMP))
		Enable(false);

	if (IsEnabled())
	{
		m_constrainUVC = (m_uvMin.u != 0 || m_uvMin.v != 0 || m_uvMax.u != 1 || m_uvMax.v != 1);
		if (m_constrainUVC)
			m_constrainRange = UVC(m_uvMax.u - m_uvMin.u, m_uvMax.v - m_uvMin.v);
	}
}

void CTexture::PostRender(void)
{
	if (m_pbumpmap != NULL)
		delete m_pbumpmap;
	m_pbumpmap = m_pmap = NULL;
	m_constrainUVC = false;
}

UVC CTexture::GetConstrainedUVC(const UVC& uv) const
{
	// slower version used outside of rendering
	if (m_uvMin.u != 0 || m_uvMin.v != 0 || m_uvMax.u != 1 || m_uvMax.v != 1)
	{
		UVC constrainRange(m_uvMax.u - m_uvMin.u, m_uvMax.v - m_uvMin.v);
		return UVC(m_uvMin.u + uv.u * constrainRange.u, m_uvMin.v + uv.v * constrainRange.v);
	}
	return uv;
}

UVC CTexture::GetConstrainedUVCFast(const UVC& uv) const
{
	// faster version used during rendering
	if (!m_constrainUVC)
		return uv;
	return UVC(m_uvMin.u + uv.u * m_constrainRange.u, m_uvMin.v + uv.v * m_constrainRange.v);
}

bool CTexture::GetTexel(const UVC& uv, TextureFilter filter, COLOR& col) const
{
	if (m_pmap && m_pmap->GetTexel(GetConstrainedUVCFast(uv), filter, col))
	{
		if (m_flags & TXTF_ALPHA)
			col.Init(m_flags & TXTF_INVERT ? 1 - col.a : col.a);
		else if (m_flags & TXTF_INVERT)
			col.Invert();
		return true;
	}
	return false;
}

bool CTexture::GetAlphaTexel(const UVC& uv, TextureFilter filter, double& alpha) const
{
	COLOR col;
	if (m_pmap && m_pmap->GetTexel(GetConstrainedUVCFast(uv), filter, col))
	{
		if (m_flags & TXTF_ALPHA)
			alpha = col.a;
		else if (m_pmap->GetFormat() == ImageFormat::GreyScale)
			alpha = col.r;
		else
			alpha = (col.r + col.g + col.b) / 3;
		if (m_flags & TXTF_INVERT)
			alpha = 1 - alpha;
		return true;
	}
	return false;
}

bool CTexture::GetBumpVector(const UVC& uv, TextureFilter filter, CPt& offset) const
{
	COLOR col;
	if (m_pbumpmap && m_pbumpmap->GetTexel(GetConstrainedUVCFast(uv), filter, col))
	{
		offset.x = -m_bscale*(col.r - 0.5);
		offset.y = -m_bscale*(col.g - 0.5);
		offset.z = 0;
		return true;
	}
	return false;
}

bool CTexture::GetAutoBumpVector(const CPt& hitpt, CPt& offset) const
{
	double r = 10000*(2.34*hitpt.x*hitpt.x + 3.45*hitpt.y*hitpt.y + 4.56*hitpt.z*hitpt.z);
	int i = (r > 0 ? (int) r%25 : (int) r%25 + 25);
	offset.x = m_bscale*(-1 + (i%5)*0.5)/100;
	offset.y = m_bscale*(-1 + (i/5)*0.5)/100;
	offset.z = 0;

	return true;
}

//-----------------------------------------------------------------------------
// CPolyTexture
//-----------------------------------------------------------------------------

CPolyTexture::CPolyTexture(void)
{
}

void CPolyTexture::Delete(void)
{
	CTexture::Delete();
	m_coords.DeleteAll();
}

TextureMapType CPolyTexture::Load(CFileBase& fobj)
{
	FILE_PTEXTURE fpt;
	if (!fobj.ReadNextBlock((byte*) &fpt, sizeof(FILE_PTEXTURE)))
		return TextureMapType::None;
	m_map = fpt.base.map;
	m_flags = fpt.base.flags;
	m_bscale = fpt.base.bscale;
	m_btol = fpt.base.btol;
	m_op = fpt.base.op;
	m_uvMin = fpt.base.uvMin;
	m_uvMax = fpt.base.uvMax;

	if (fobj.ReadNextBlockType() != BlockType::PTextureCoords)
		return TextureMapType::None;

	UVC* pcoords = m_coords.LockArray(fpt.num_coords);
	int ret = fobj.ReadNextBlock((byte*) pcoords, fpt.num_coords*sizeof(UVC));
	m_coords.UnlockArray();

	return (ret ? fpt.base.type : TextureMapType::None);
}

void CPolyTexture::Save(CFileBase& fobj, TextureMapType type)
{
	FILE_PTEXTURE fpt;
	fpt.base.type = type;
	fpt.base.flags = m_flags;
	if (sizeof(fpt.base.map) <= m_map.length())
		throw fileio_exception("Poly texture map name too long: " + m_map);
	strncpy(fpt.base.map, m_map.c_str(), sizeof(fpt.base.map));
	fpt.base.btol = m_btol;
	fpt.base.bscale = m_bscale;
	fpt.base.op = m_op;
	fpt.base.uvMin = m_uvMin;
	fpt.base.uvMax = m_uvMax;

	fpt.num_coords = m_coords.CountLoaded();
	fpt.reserved = 0;
	if (!fobj.WriteDataBlock(BlockType::PTexture, (byte*) &fpt, sizeof(FILE_PTEXTURE)))
		throw fileio_exception("Unable to write poly texture");

	dword size = fpt.num_coords*sizeof(UVC);
	if (!fobj.WriteDataBlock(BlockType::PTextureCoords, (byte*) &(m_coords[0]), size))
		throw fileio_exception("Unable to write poly texture coordinates");
}

bool CPolyTexture::LoadCoords(const UVC* uvcs, int num)
{
	m_flags &= ~TXTF_POLY_INDC;
	for (int n = 0; n < num; n++)
		LoadCoord(uvcs[n]);
	return LoadComplete();
}

bool CPolyTexture::LoadCoord(const UVC& uvc)
{
	return m_coords.LoadObj(uvc);
}

bool CPolyTexture::LoadComplete(void)
{
	return m_coords.LoadComplete();
}

void CPolyTexture::ChangeCoord(int index, const UVC& uvc)
{
	m_coords[index] = uvc;
}

void CPolyTexture::DeleteCoords(void)
{
	m_coords.DeleteAll();
}

int CPolyTexture::Count(void)
{
	return m_coords.Count();
}

//-----------------------------------------------------------------------------
// CPolygon (texture wrapping routines)
//-----------------------------------------------------------------------------

static UVC ComputeSphericalCoord(const CPt& pt, const CPt& center, const CBoundBox& bbox)
{
	UVC final;

	// this code would shape the spherical map around the object's bounding box
	/*CPt min = bbox.GetMinPt();
	CPt max = bbox.GetMaxPt();
	double sx = max.x - min.x;
	double sy = max.y - min.y;
	double sz = max.z - min.z;
	double maxd = max(sx, max(sy, sz));
	sx = sx / maxd;
	sy = sy / maxd;
	sz = sz / maxd;*/

	CUnitVector norm = pt - center;
	/*norm.x *= sx;
	norm.y *= sy;
	norm.z *= sz;*/
	norm.Normalize();

	CUnitVector tmp = norm;
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
	final.v = (1 + norm.y) / 2.0;

	return final;
}

static UVC ComputeEllipticalCoord(const CPt& pt, const CPt& center, const CBoundBox& bbox, const CUnitVector& axis)
{
	UVC final;

	CPt min = bbox.GetMinPt();
	CPt max = bbox.GetMaxPt();

	CUnitVector norm = pt - center;
	if (axis.x != 0)
		norm = CUnitVector(norm.y, norm.x, norm.z);
	else if (axis.y != 0)
		norm = CUnitVector(norm.x, norm.y, norm.z);
	else
		norm = CUnitVector(norm.x, norm.z, norm.y);
	norm.Normalize();

	CUnitVector tmp = norm;
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
	final.v = (pt.y-min.y) / (max.y-min.y);

	return final;
}

/*static UVC ComputeProjectionCoord(const CPt& pt, const CBoundBox& bbox, const CUnitVector& axis)
{
	CPt min = bbox.GetMinPt();
	CPt max = bbox.GetMaxPt();
	if (axis.x != 0)
		return UVC((pt.y-min.y) / (max.y-min.y), (pt.z-min.z) / (max.z-min.z));
	else if (axis.y != 0)
		return UVC((pt.x-min.x) / (max.x-min.x), (pt.z-min.z) / (max.z-min.z));
	return UVC((pt.x-min.x) / (max.x-min.x), (pt.y-min.y) / (max.y-min.y));
}*/

// attempt to correct for aspect ratio of the object
static UVC ComputeProjectionCoord(const CPt& pt, const CBoundBox& bbox, const CUnitVector& axis)
{
	UVC ret;
	double aspect;

	CPt min = bbox.GetMinPt();
	CPt max = bbox.GetMaxPt();
	if (axis.x != 0)
	{
		ret = UVC((pt.y-min.y) / (max.y-min.y), (pt.z-min.z) / (max.z-min.z));
		aspect = (max.y - min.y) / (max.z - min.z);
	}
	else if (axis.y != 0)
	{
		ret = UVC((pt.x-min.x) / (max.x-min.x), (pt.z-min.z) / (max.z-min.z));
		aspect = (max.x - min.x) / (max.z - min.z);
	}
	else
	{
		ret = UVC((pt.x-min.x) / (max.x-min.x), (pt.y-min.y) / (max.y-min.y));
		aspect = (max.x - min.x) / (max.y - min.y);
	}

	if (aspect < 1)
		ret.u = 0.5 - (0.5 - ret.u) * aspect;
	else if (aspect > 1)
		ret.v = 0.5 - (0.5 - ret.v) / aspect;
	return ret;
}

static UVC ComputeMappingCoord(TextureMapWrapType wtype, const CPt& pt, const CPt& center, const CBoundBox& bbox, const CUnitVector& axis, UVC* uvmin, UVC* uvmax)
{
	UVC final;
	if (wtype == TextureMapWrapType::Spherical)
		final = ComputeSphericalCoord(pt, center, bbox);
	else if (wtype == TextureMapWrapType::Elliptical)
		final = ComputeEllipticalCoord(pt, center, bbox, axis);
	else
		final = ComputeProjectionCoord(pt, bbox, axis);

	if (uvmin != NULL && uvmax != NULL)
	{
		double du = uvmax->u - uvmin->u;
		double dv = uvmax->v - uvmin->v;
		final.u = uvmin->u + du*final.u;
		final.v = uvmin->v + dv*final.v;
	}

	return final;
}

bool CPolygon::ApplyMapping(TextureMapWrapType wtype, TextureMapType type, int map, bool indc, CPt* pcenter, CUnitVector* paxis, UVC* uvmin, UVC* uvmax)
{
	PTEXTURES pmaps = GetTextures(type);
	if (pmaps == NULL || type == TextureMapType::Reflection)
		throw model_exception("Texture map type invalid");
	if (map == -1 || map >= (int) pmaps->size())
		throw model_exception("Invalid texture map index");
	if (!indc && wtype == TextureMapWrapType::Extrusion)
		throw model_exception("Extrusion wrap mapping requires indc");

	int npts = m_lattice.CountLoaded();

	CBoundBox bbox;
	bbox.StartNewBox();
	for (int n = 0; n < npts; n++)
		bbox.AddPoint(m_lattice[n]);
	bbox.FinishNewBox();

	CPt center = (pcenter != NULL ? *pcenter : bbox.GetCenter());
	CUnitVector axis = (paxis != NULL ? *paxis : CUnitVector(0, 0, 1));

	CPolyTexture* ptxt = (CPolyTexture*) (*pmaps)[map].get();
	if (ptxt != NULL)
	{
		ptxt->DeleteCoords();

		if (!indc)
		{
			vector<UVC> uvcs(npts);
			for (int n = 0; n < npts; n++)
			{
				uvcs[n] = ComputeMappingCoord(wtype, m_lattice[n], center, bbox, axis, uvmin, uvmax);
			}

			ptxt->LoadCoords(uvcs.data(), npts);
		}
		else
		{
			for (int n = 0; n < m_curves.Count(); n++)
			{
				POLY_CURVE& pc = m_curves[n];
				if (pc.flags & (PCF_INVISIBLE))
					continue;
				if (pc.smap == -1)
					pc.smap = ptxt->Count();

				// extrusion mapping is actually a combination of projection and spherical mapping
				TextureMapWrapType nwtype = wtype;
				if (wtype == TextureMapWrapType::Extrusion)
					nwtype = (fabs(pc.norm.DotProduct(axis)) >= 0.9 ? TextureMapWrapType::Projection : TextureMapWrapType::Spherical);

				for (int c = 0; c < pc.cnt; c++)
				{
					UVC uvc = ComputeMappingCoord(nwtype, m_lattice[m_ind[pc.sind+c]], center, bbox, axis, uvmin, uvmax);
					ptxt->LoadCoord(uvc);
				}
			}

			ptxt->LoadComplete();
		}
	}

	return true;
}
