// polygon.cpp - defines the CPolygon class
//

#include "polygon.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CPolygon
//-----------------------------------------------------------------------------

CPolygon::CPolygon(void) : m_doedges(false)
{
}

CPolygon::~CPolygon(void)
{
	DeleteAll();
}

void CPolygon::operator=(const CPolygon& rhs)
{
	CObj::Duplicate(rhs);

	m_lattice = rhs.m_lattice;
	m_curves = rhs.m_curves;
	m_ind = rhs.m_ind;
	m_norms = rhs.m_norms;
}

shared_ptr<CTexture> CPolygon::DupTexture(TextureMapType type, const shared_ptr<CTexture>& rhs)
{
	if (type == TextureMapType::Reflection)
		return CObj::DupTexture(type, rhs);
	auto pnew = make_shared<CPolyTexture>();
	*pnew = *(static_cast<CPolyTexture*>(rhs.get()));
	return pnew;
}

bool CPolygon::LoadLattice(int nlattice, const CPt* pts)
{
	return m_lattice.LoadDirect(pts, nlattice);
}

int CPolygon::LoadLatticePt(const CPt& pt)
{
	m_lattice.LoadObj(pt);
	return (m_lattice.Count() - 1);
}

bool CPolygon::AddPolyCurve(const CUnitVector& norm, dword flags, int* indarray, CUnitVector* ptnorms, int nind)
{
	POLY_CURVE pc;
	memset(&pc, 0, sizeof(POLY_CURVE));
	pc.norm = norm;
	pc.flags = flags;
	pc.sind = m_ind.Count();
	pc.snorm = m_norms.Count();
	pc.smap = -1;
	pc.cnt = (indarray != NULL ? nind : 0);
	pc.prp = NULL;

	if (indarray != NULL && nind > 0)
	{
		for (int n = 0; n < nind; n++)
		{
			m_ind.LoadObj(indarray[n]);
		}
	}

	if (ptnorms != NULL && nind > 0)
	{
		for (int n = 0; n < nind; n++)
		{
			m_norms.LoadObj(ptnorms[n]);
		}
	}

	m_curves.LoadObj(pc);

	return true;
}

bool CPolygon::AddPolyCurveIndex(int index)
{
	int size = m_curves.Count();
	if (size > 0)
	{
		POLY_CURVE& pc = m_curves[size-1];
		pc.cnt++;

		m_ind.LoadObj(index);
	}
	return (size > 0);
}

bool CPolygon::AddPolyCurveNormal(const CUnitVector& ptnorm)
{
	// note that this call must always come after the AddPolyCurveIndex() call
	return m_norms.LoadObj(ptnorm);
}

int CPolygon::AddColorMap(TextureMapType type, TextureMapOp op, dword flags, const string& map, UVC* uvcs, int num, UVC uvMin, UVC uvMax)
{
	if (num > 0 && num != m_lattice.Count())
		return -1;

	if (type == TextureMapType::Diffuse || type == TextureMapType::Specular || type == TextureMapType::Refraction || type == TextureMapType::Glow)
	{
		auto pnew = make_shared<CPolyTexture>();
		flags |= TXTF_ENABLED;
		if (num == 0)
			flags |= TXTF_POLY_INDC;
		pnew->Init(map, flags, op, uvMin, uvMax);
		if (num > 0)
			pnew->LoadCoords(uvcs, num);

		return CObj::AddTextureMap(type, pnew);
	}

	return -1;
}

int CPolygon::AddTransparencyMap(dword flags, const string& map, UVC* uvcs, int num, UVC uvMin, UVC uvMax)
{
	if (num > 0 && num != m_lattice.Count())
		return -1;

	auto pnew = make_shared<CPolyTexture>();
	flags |= TXTF_ENABLED;
	if (num == 0)
		flags |= TXTF_POLY_INDC;
	pnew->Init(map, flags, TextureMapOp::Multiply, uvMin, uvMax);
	if (num > 0)
		pnew->LoadCoords(uvcs, num);

	return CObj::AddTextureMap(TextureMapType::Transparency, pnew);
}

int CPolygon::AddBumpMap(dword flags, const string& map, double bscale, int btol, UVC* uvcs, int num, UVC uvMin, UVC uvMax)
{
	if (num > 0 && num != m_lattice.Count())
		return -1;

	auto pnew = make_shared<CPolyTexture>();
	flags |= (TXTF_ENABLED | TXTF_IS_BUMP);
	if (num == 0)
		flags |= TXTF_POLY_INDC;
	pnew->Init(map, flags, TextureMapOp::Multiply, uvMin, uvMax);
	pnew->SetBumpParams(bscale, btol);
	if (num > 0)
		pnew->LoadCoords(uvcs, num);

	return CObj::AddTextureMap(TextureMapType::Bump, pnew);
}

bool CPolygon::AddPolyCurveMapCoord(TextureMapType type, int mindex, const UVC& uvc)
{
	PTEXTURES pmaps = GetTextures(type);
	if (pmaps == NULL)
		return false;
	if (mindex == -1 || mindex >= (int) pmaps->size())
		return false;

	int size = m_curves.Count();
	if (size > 0)
	{
		POLY_CURVE& pc = m_curves[size-1];
		if (pc.smap == -1)
			pc.smap = (static_cast<CPolyTexture*>((*pmaps)[mindex].get()))->Count();
	}

	return (static_cast<CPolyTexture*>((*pmaps)[mindex].get()))->LoadCoord(uvc);
}

bool CPolygon::LoadComplete(void)
{
	m_lattice.LoadComplete();
	m_curves.LoadComplete();
	m_ind.LoadComplete();
	m_norms.LoadComplete();

	for (const auto& iter : m_dmaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();
	for (const auto& iter : m_smaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();
	for (const auto& iter : m_fmaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();
	for (const auto& iter : m_gmaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();
	for (const auto& iter : m_tmaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();
	for (const auto& iter : m_bmaps)
		(static_cast<CPolyTexture*>(iter.get()))->LoadComplete();

	return true;
}

void CPolygon::DeleteAll(void)
{
	m_lattice.DeleteAll();
	m_curves.DeleteAll();
	m_ind.DeleteAll();
	m_norms.DeleteAll();

	CObj::RemoveAllMaps();
}

const CLoadArray<CPt>& CPolygon::GetLattice(void) const
{
	return m_lattice;
}

const CLoadArray<POLY_CURVE>& CPolygon::GetCurves(void) const
{
	return m_curves;
}

const CLoadArray<int>& CPolygon::GetIndices(void) const
{
	return m_ind;
}

const CLoadArray<CUnitVector>& CPolygon::GetNormals(void) const
{
	return m_norms;
}

void CPolygon::GetBoundBox(const CMatrix& tm, CBoundBox& bbox) const
{
	// compute the bounding box (global coords)
	bbox.StartNewBox();
	for (int n = 0; n < m_lattice.CountLoaded(); n++)
	{
		CPt pt = m_lattice[n];
		tm.TransformPoint(pt, 1, MatrixType::CTM);
		bbox.AddPoint(pt);
	}
	bbox.FinishNewBox();
}

void CPolygon::LoadMap(BlockType bt, CFileBase& fobj)
{
	if (bt == BlockType::PTexture)
	{
		auto pnew = make_shared<CPolyTexture>();
		TextureMapType type = pnew->Load(fobj);
		if (type == TextureMapType::None)
			throw fileio_exception("Invalid texture map type");
		CObj::AddTextureMap(type, pnew);
	}
	else
		CObj::LoadMap(bt, fobj);
}

void CPolygon::Load(CFileBase& fobj)
{
	FILE_POLYGON fp;
	if (!fobj.ReadNextBlock((byte*) &fp, sizeof(FILE_POLYGON)))
		throw fileio_exception("Unable to read polygon block");
	m_diff = fp.base.diff;
	m_spec = fp.base.spec;
	m_refl = fp.base.refl;
	m_refr = fp.base.refr;
	m_glow = fp.base.glow;
	m_index = fp.base.index;
	m_flags = fp.base.flags;
	m_tm = fp.base.tm;
	m_filter = fp.base.filter;

	if (fp.num_lattice > 0)
	{
		if (fobj.ReadNextBlockType() != BlockType::PolyLattice)
			throw fileio_exception("Failed to load polygon lattice");

		CPt* plattice = m_lattice.LockArray(fp.num_lattice);
		int ret = fobj.ReadNextBlock((byte*) plattice, fp.num_lattice*sizeof(CPt));
		m_lattice.UnlockArray();
		if (!ret)
			throw fileio_exception("Failed to load polygon lattice");
	}

	if (fp.num_curves > 0)
	{
		if (fobj.ReadNextBlockType() != BlockType::PolyCurves)
			throw fileio_exception("Failed to load polygon curves");

        CLoadArray<POLY_CURVE_t> loadCurves;
        POLY_CURVE_t* plcurves = loadCurves.LockArray(fp.num_curves);
        int ret = fobj.ReadNextBlock((byte*) plcurves, fp.num_curves*sizeof(POLY_CURVE_t));
        if (ret)
        {
            POLY_CURVE* pcurves = m_curves.LockArray(fp.num_curves);
            for (int i = 0; i < fp.num_curves; i++) {
                memcpy((void*) pcurves, (void*) plcurves, sizeof(POLY_CURVE_t));
                pcurves->prp = nullptr;

                pcurves++;
                plcurves++;
            }
            m_curves.UnlockArray();
        }
        loadCurves.UnlockArray();
        if (!ret)
			throw fileio_exception("Failed to load polygon curves");
	}

	if (fp.num_ind > 0)
	{
		if (fobj.ReadNextBlockType() != BlockType::PolyIndices)
			throw fileio_exception("Failed to load polygon indices");

		int* pinds = m_ind.LockArray(fp.num_ind);
		int ret = fobj.ReadNextBlock((byte*) pinds, fp.num_ind*sizeof(int));
		m_ind.UnlockArray();
		if (!ret)
			throw fileio_exception("Failed to load polygon indices");
	}

	if (fp.num_norms > 0)
	{
		if (fobj.ReadNextBlockType() != BlockType::PolyNorms)
			throw fileio_exception("Failed to load polygon normals");

		CUnitVector* pnorms = m_norms.LockArray(fp.num_norms);
		int ret = fobj.ReadNextBlock((byte*) pnorms, fp.num_norms*sizeof(CUnitVector));
		m_norms.UnlockArray();
		if (!ret)
			throw fileio_exception("Failed to load polygon normals");
	}

	CObj::Load(fobj);
}

void CPolygon::Save(CFileBase& fobj)
{
	FILE_POLYGON fp;
	fp.base.diff = m_diff;
	fp.base.spec = m_spec;
	fp.base.refl = m_refl;
	fp.base.refr = m_refr;
	fp.base.glow = m_glow;
	fp.base.index = m_index;
	fp.base.flags = m_flags;
	fp.base.tm = m_tm;
	fp.base.filter = m_filter;

	fp.num_lattice = m_lattice.CountLoaded();
	fp.num_curves = m_curves.CountLoaded();
	fp.num_ind = m_ind.CountLoaded();
	fp.num_norms = m_norms.CountLoaded();
	if (!fobj.WriteDataBlock(BlockType::Polygon, (byte*) &fp, sizeof(FILE_POLYGON)))
		throw fileio_exception("Failed to write polygon header");

	if (fp.num_lattice > 0)
	{
		dword size = fp.num_lattice*sizeof(CPt);
		if (!fobj.WriteDataBlock(BlockType::PolyLattice, (byte*) &(m_lattice[0]), size))
			throw fileio_exception("Failed to write polygon lattice");
	}

	if (fp.num_curves > 0)
	{
		dword size = fp.num_curves*sizeof(POLY_CURVE);
		if (!fobj.WriteDataBlock(BlockType::PolyCurves, (byte*) &(m_curves[0]), size))
			throw fileio_exception("Failed to write polygon curves");
	}

	if (fp.num_ind > 0)
	{
		dword size = fp.num_ind*sizeof(int);
		if (!fobj.WriteDataBlock(BlockType::PolyIndices, (byte*) &(m_ind[0]), size))
			throw fileio_exception("Failed to write polygon indices");
	}

	if (fp.num_norms > 0)
	{
		dword size = fp.num_norms*sizeof(CUnitVector);
		if (!fobj.WriteDataBlock(BlockType::PolyNorms, (byte*) &(m_norms[0]), size))
			throw fileio_exception("Failed to write polygon normals");
	}

	CObj::Save(fobj);
}

void CPolygon::RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images)
{
	CObj::RenderStart(nthreads, itm, cam, images);

	vector<SCREENPOS> posv;
	if (UseBoundBox())
	{
		// compute the bounding box (global coords)
		m_sbox.StartNewBox();
		for (int n = 0; n < m_lattice.CountLoaded(); n++)
		{
			CPt pt = m_lattice[n];
			m_ftm.TransformPoint(pt, 1, MatrixType::CTM);

			SCREENPOS pos;
			if (cam.GlobalToScreen(pt, pos))
				m_sbox.AddScreenPoint(pos);
			posv.push_back(pos);
		}
		m_sbox.FinishNewBox();
	}

	// setup temporary rendering variables in the curve list
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		POLY_CURVE& pc = m_curves[n];
		pc.prp = new POLY_CURVE_REND;
		pc.prp->norigcurve = pc.prp->npoly = 0;

		// compute polygon distance to origin (local coords)
		CUnitVector tmp(m_lattice[m_ind[pc.sind]]);
		pc.prp->fd = -tmp.DotProduct(pc.norm);

		// compute which coord to discard when doing hit tests against this poly
		CPt absnorm(fabs(pc.norm.x), fabs(pc.norm.y), fabs(pc.norm.z));
		if (absnorm.x >= absnorm.y && absnorm.x >= absnorm.z) pc.prp->discard = 0;
		else if (absnorm.y >= absnorm.x && absnorm.y >= absnorm.z) pc.prp->discard = 1;
		else if (absnorm.z >= absnorm.x && absnorm.z >= absnorm.y) pc.prp->discard = 2;

		// build screen bounding rects for each poly
		if (UseBoundBox())
		{
			pc.prp->scrbox.StartNewBox();
			for (int n = 0; n < pc.cnt; n++)
				pc.prp->scrbox.AddScreenPoint(posv[m_ind[pc.sind+n]]);
			pc.prp->scrbox.FinishNewBox();
		}
	}

	// do we need to do edge tracking for this object
	m_doedges = false;
	TextureMapType types[] = {
		TextureMapType::Diffuse,
		TextureMapType::Specular,
		TextureMapType::Refraction,
		TextureMapType::Glow,
		TextureMapType::Transparency,
		TextureMapType::Bump };
	for (int i = 0; !m_doedges && i < sizeof(types) / sizeof(TextureMapType); i++)
	{
		const PTEXTURES pmaps = GetTextures(types[i]);
		if (pmaps)
		{
			for (int n = 0; !m_doedges && n < (int) pmaps->size(); n++)
			{
				if ((*pmaps)[n]->IsEnabled())
					m_doedges = true;
			}
		}
	}

	m_prp.resize(nthreads);
}

void CPolygon::RenderFinish(void)
{
	// clean up the temporary rendering params
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		POLY_CURVE& pc = m_curves[n];
		delete pc.prp;
		pc.prp = NULL;
	}
	m_prp.clear();

	CObj::RenderFinish();
}

bool CPolygon::IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr)
{
	if (IsInvisible() || m_curves.CountLoaded() == 0)
		return false;

	bool usebbox = UseBoundBox();
	if (usebbox)
	{
		// we either use screen or world bounding boxes
		if (m_sbox.IsValid(ray.pos))
		{
			if (!m_sbox.IsInRect(ray.pos))
				return false;
		}
		else if (!m_bbox.IntersectRay(ray))
			return false;
	}

	// transform the incoming ray into local coords
	intr.lray = ray;
	double len = m_ftm.TransformRay(intr.lray, MatrixType::ICTM);
	intr.llen = intr.glen = -1;

	int hits = 0;
	REND_PARAMS trp;
	REND_PARAMS& crp = m_prp[nthread];
	crp.hitlen = trp.hitlen = -1;
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		const POLY_CURVE& pc = m_curves[n];

		if (pc.flags & PCF_NEWPLANE)
		{
			// did the last curve(s) hit?
			if (trp.hitlen != -1)
			{
				// yup, is it the new closest hit?
				if (hits%2 == 1 && (crp.hitlen == -1 || trp.hitlen < crp.hitlen))
				{
					crp = trp;
				}
			}

			trp.hitlen = -1;
			hits = 0;
		}

		// if this is a camera generated ray, and it isn't in the curve's screen box, move on
		if (usebbox && !pc.prp->scrbox.IsInRect(ray.pos))
			continue;

		// if the poly is invisible, move on
		if (pc.flags & PCF_INVISIBLE)
			continue;

		if (trp.hitlen == -1)
		{
			trp.norm = pc.norm;
			trp.flags = pc.flags;
			trp.redge.dist = trp.ledge.dist = -1;

			// this curve is a new plane, so recompute the hit distance
			double vd = trp.norm.DotProduct(intr.lray.dir);
			if (vd == 0)
				continue;
			double vo = -(trp.norm.DotProduct(intr.lray.ori) + pc.prp->fd);
			trp.hitlen = vo / vd;

			// compute the hit point (EPSILON can overcome double precision errors, but beware!)
			if (trp.hitlen > 0)
				trp.hitpt = intr.lray.ori + intr.lray.dir*(trp.hitlen + EPSILON);
		}

		// if backface culling is on, check it
		if ((pc.flags & PCF_CULL) && trp.norm.DotProduct(intr.lray.dir) > 0)
			continue;

		// if the hit distance is negative, that counts as a miss
		if (trp.hitlen <= 0)
			continue;

		bool isrlastpos = false, isllastpos = false;
		bool isrfirstsegmentpos = false, islfirstsegmentpos = false;
		for (int i = 0; i < pc.cnt; i++)
		{
			int iv1 = m_ind[pc.sind + i];
			int iv2 = m_ind[pc.sind + (i+1)%pc.cnt];

			// collapse the 2 points by dropping a coordinate
			double a[2], b[2];
			switch (pc.prp->discard)
			{
			case 0:
				a[0] = m_lattice[iv1].y - trp.hitpt.y;
				a[1] = m_lattice[iv1].z - trp.hitpt.z;
				b[0] = m_lattice[iv2].y - trp.hitpt.y;
				b[1] = m_lattice[iv2].z - trp.hitpt.z;
				break;
			case 1:
				a[0] = m_lattice[iv1].x - trp.hitpt.x;
				a[1] = m_lattice[iv1].z - trp.hitpt.z;
				b[0] = m_lattice[iv2].x - trp.hitpt.x;
				b[1] = m_lattice[iv2].z - trp.hitpt.z;
				break;
			case 2:
				a[0] = m_lattice[iv1].x - trp.hitpt.x;
				a[1] = m_lattice[iv1].y - trp.hitpt.y;
				b[0] = m_lattice[iv2].x - trp.hitpt.x;
				b[1] = m_lattice[iv2].y - trp.hitpt.y;
				break;
			}

			// does the edge even cross the X axis?
			if ((a[1] != 0 || b[1] != 0) && ((a[1] >= 0 && b[1] <= 0) || (a[1] <= 0 && b[1] >= 0)))
			{
				bool rhit = false, lhit = false;

				// does it cross to the right of the ray intersection point?
				double ulen = (-a[1] / (b[1] - a[1]));
				double x = a[0] + ulen*(b[0] - a[0]);
				if (x > 0)
				{
					if ((a[1] > 0 && b[1] < 0) || (a[1] < 0 && b[1] > 0))
						rhit = true;
					else if (b[1] == 0)
					{
						if (i == pc.cnt - 1)
						{
							// special case - last point of the last segment hit exactly
							if ((a[1] > 0 && !isrfirstsegmentpos) || (a[1] < 0 && isrfirstsegmentpos))
								rhit = true;
						}
						else
						{
							// special case - the second point hit exactly
							if (a[1] > 0)
								isrlastpos = true;
							else if (a[1] < 0)
								isrlastpos = false;
						}
					}
					else if (a[1] == 0)
					{
						if (i == 0)
						{
							// special case - the first point of the first segment hit exactly
							if (b[1] > 0)
								isrfirstsegmentpos = true;
							else if (b[1] < 0)
								isrfirstsegmentpos = false;
						}
						else
						{
							// special case - the first point hit exactly
							if ((b[1] > 0 && !isrlastpos) || (b[1] < 0 && isrlastpos))
								rhit = true;
						}
					}
				}
				else if (m_doedges || (pc.flags & PCF_VERTEXNORMS))
				{
					// exactly the same as above, but for the left side of the intersection point
					if ((a[1] > 0 && b[1] < 0) || (a[1] < 0 && b[1] > 0))
						lhit = true;
					else if (b[1] == 0)
					{
						if (i == pc.cnt - 1)
						{
							// special case - last point of the last segment hit exactly
							if ((a[1] > 0 && !islfirstsegmentpos) || (a[1] < 0 && islfirstsegmentpos))
								lhit = true;
						}
						else
						{
							// special case - the second point hit exactly
							if (a[1] > 0)
								isllastpos = true;
							else if (a[1] < 0)
								isllastpos = false;
						}
					}
					else if (a[1] == 0)
					{
						if (i == 0)
						{
							// special case - the first point of the first segment hit exactly
							if (b[1] > 0)
								islfirstsegmentpos = true;
							else if (b[1] < 0)
								islfirstsegmentpos = false;
						}
						else
						{
							// special case - the first point hit exactly
							if ((b[1] > 0 && !isllastpos) || (b[1] < 0 && isllastpos))
								lhit = true;
						}
					}
				}

				if (rhit)
				{
					hits++;
					if (!(pc.flags & PCF_CW) && (trp.redge.dist == -1 || x < trp.redge.dist))
					{
						trp.redge.pcind = n;
						trp.redge.cind = i;
						trp.redge.ulen = ulen;
						trp.redge.dist = x;
					}
				}
				else if (lhit)
				{
					x = -x;
					if (!(pc.flags & PCF_CW) && (trp.ledge.dist == -1 || x < trp.ledge.dist))
					{
						trp.ledge.pcind = n;
						trp.ledge.cind = i;
						trp.ledge.ulen = ulen;
						trp.ledge.dist = x;
					}
				}
			}
        }
	}

	// did the last curve(s) hit?
	if (trp.hitlen != -1)
	{
		// yup, is it the new closest hit?
		if (hits%2 == 1 && (crp.hitlen == -1 || trp.hitlen < crp.hitlen))
		{
			crp = trp;
		}
	}

	// convert the hit length to global coords
	if (crp.hitlen > 0)
	{
		intr.glen = intr.llen = crp.hitlen;
		if (len != 1)
			intr.glen *= len;
	}

	// this version filters out messed up clockedness in some TT fonts, but the face will appear transparent
	//return (intr.llen > 0 && crp.ledge.dist != -1 && crp.redge.dist != -1);
	return (intr.llen > 0);
}

// TODO: do I want this to share any code with IntersectRay()?
bool CPolygon::IntersectShadowRay(int nthread, const CRay& ray, double max)
{
	if (!IsShadowCaster())
		return false;

	if (UseBoundBox() && !m_bbox.IntersectRay(ray))
		return false;

	// transform the incoming ray into local coords
	CRay lray = ray;
	double len = m_ftm.TransformRay(lray, MatrixType::ICTM);

	int hits = 0;
	REND_PARAMS trp;
	trp.hitlen = -1;
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		const POLY_CURVE& pc = m_curves[n];

		if (pc.flags & PCF_NEWPLANE)
		{
			// did the last curve(s) hit?
			if (trp.hitlen != -1)
			{
				// yup, is it the new closest hit?
				if (hits%2 == 1 && (max == -1 || len*trp.hitlen < max))
					return true;
			}

			trp.hitlen = -1;
			hits = 0;
		}

		// if the poly is invisible, move on
		if (pc.flags & PCF_INVISIBLE)
			continue;

		if (trp.hitlen == -1)
		{
			// this curve is a new plane, so recompute the hit distance
			double vd = pc.norm.DotProduct(lray.dir);
			if (vd == 0)
				continue;
			double vo = -(pc.norm.DotProduct(lray.ori) + pc.prp->fd);
			trp.hitlen = vo / vd;

			// compute the hit point (EPSILON can overcome double precision errors, but beware!)
			if (trp.hitlen > 0)
				trp.hitpt = lray.ori + lray.dir*(trp.hitlen + EPSILON);
		}

		// if backface culling is on, check it
		if ((pc.flags & PCF_CULL) && pc.norm.DotProduct(lray.dir) > 0)
			continue;

		// if the hit distance is negative, that counts as a miss
		if (trp.hitlen <= 0)
			continue;

		bool isrlastpos = false, isllastpos = false;
		bool isrfirstsegmentpos = false, islfirstsegmentpos = false;
		for (int i = 0; i < pc.cnt; i++)
		{
			int iv1 = m_ind[pc.sind + i];
			int iv2 = m_ind[pc.sind + (i+1)%pc.cnt];

			// collapse the 2 points by dropping a coordinate
			double a[2], b[2];
			switch (pc.prp->discard)
			{
			case 0:
				a[0] = m_lattice[iv1].y - trp.hitpt.y;
				a[1] = m_lattice[iv1].z - trp.hitpt.z;
				b[0] = m_lattice[iv2].y - trp.hitpt.y;
				b[1] = m_lattice[iv2].z - trp.hitpt.z;
				break;
			case 1:
				a[0] = m_lattice[iv1].x - trp.hitpt.x;
				a[1] = m_lattice[iv1].z - trp.hitpt.z;
				b[0] = m_lattice[iv2].x - trp.hitpt.x;
				b[1] = m_lattice[iv2].z - trp.hitpt.z;
				break;
			case 2:
				a[0] = m_lattice[iv1].x - trp.hitpt.x;
				a[1] = m_lattice[iv1].y - trp.hitpt.y;
				b[0] = m_lattice[iv2].x - trp.hitpt.x;
				b[1] = m_lattice[iv2].y - trp.hitpt.y;
				break;
			}

			// does the edge even cross the X axis?
			if ((a[1] != 0 || b[1] != 0) && ((a[1] >= 0 && b[1] <= 0) || (a[1] <= 0 && b[1] >= 0)))
			{
				bool rhit = false;

				// does it cross to the right of the ray intersection point?
				double ulen = (-a[1] / (b[1] - a[1]));
				double x = a[0] + ulen*(b[0] - a[0]);
				if (x > 0)
				{
					if ((a[1] > 0 && b[1] < 0) || (a[1] < 0 && b[1] > 0))
						rhit = true;
					else if (b[1] == 0)
					{
						if (i == pc.cnt - 1)
						{
							// special case - last point of the last segment hit exactly
							if ((a[1] > 0 && !isrfirstsegmentpos) || (a[1] < 0 && isrfirstsegmentpos))
								rhit = true;
						}
						else
						{
							// special case - the second point hit exactly
							if (a[1] > 0)
								isrlastpos = true;
							else if (a[1] < 0)
								isrlastpos = false;
						}
					}
					else if (a[1] == 0)
					{
						if (i == 0)
						{
							// special case - the first point of the first segment hit exactly
							if (b[1] > 0)
								isrfirstsegmentpos = true;
							else if (b[1] < 0)
								isrfirstsegmentpos = false;
						}
						else
						{
							// special case - the first point hit exactly
							if ((b[1] > 0 && !isrlastpos) || (b[1] < 0 && isrlastpos))
								rhit = true;
						}
					}
				}

				if (rhit)
					hits++;
			}
        }
	}

	// did the last curve(s) hit?
	if (trp.hitlen != -1)
	{
		// yup, is it the new closest hit?
		if (hits%2 == 1 && (max == -1 || len*trp.hitlen < max))
			return true;
	}

	return false;
}

bool CPolygon::ComputeTexelCoord(int nthread, const CTexture* ptxt, UVC& final)
{
	CPolyTexture* pptxt = (CPolyTexture*) ptxt;
	if (!pptxt->IsValid())
		return false;
	bool isind = pptxt->IsInd();
	REND_PARAMS& crp = m_prp[nthread];
	if (crp.ledge.dist == -1 || crp.redge.dist == -1)
		return false;

	const POLY_CURVE& pc1 = m_curves[crp.ledge.pcind];
	int i1 = (isind ? pc1.smap : pc1.sind) + crp.ledge.cind;
	int i2 = (isind ? pc1.smap : pc1.sind) + (crp.ledge.cind + 1) % pc1.cnt;
	UVC c1 = pptxt->GetCoord(isind ? i1 : m_ind[i1]);
	UVC c2 = pptxt->GetCoord(isind ? i2 : m_ind[i2]);
	UVC diff = (c2 - c1) * crp.ledge.ulen;
	UVC fc1 = c1 + diff;

	const POLY_CURVE& pc2 = m_curves[crp.redge.pcind];
	i1 = (isind ? pc2.smap : pc2.sind) + crp.redge.cind;
	i2 = (isind ? pc2.smap : pc2.sind) + (crp.redge.cind + 1) % pc2.cnt;
	c1 = pptxt->GetCoord(isind ? i1 : m_ind[i1]);
	c2 = pptxt->GetCoord(isind ? i2 : m_ind[i2]);
	diff = (c2 - c1) * crp.redge.ulen;
	UVC fc2 = c1 + diff;

	double ulen = crp.ledge.dist / (crp.redge.dist + crp.ledge.dist);
	diff = (fc2 - fc1)*ulen;
	final = fc1 + diff;

	return true;
}

// TODO: not sure this is correct if using vertex normals, and it isn't thread safe
bool CPolygon::ComputeBumpMapToNormTM(int nthread, const CUnitVector& norm, CMatrix& tm)
{
	REND_PARAMS& crp = m_prp[nthread];
	if (!(crp.flags & PCF_VERTEXNORMS))
		return CObj::ComputeBumpMapToNormTM(nthread, norm, tm);

	POLY_CURVE& pc = m_curves[crp.ledge.pcind];
	if (pc.prp->npoly == 0)
	{
		CObj::ComputeBumpMapToNormTM(nthread, norm, pc.prp->tonormtm);
		pc.prp->npoly = 1;	// we'll reuse this (normally used by extrusion)
	}
	tm = pc.prp->tonormtm;
	return true;
}

bool CPolygon::PostIntersect(int nthread, INTERSECTION& intr)
{
	REND_PARAMS& crp = m_prp[nthread];
	if (crp.flags & PCF_VERTEXNORMS)
	{
		// interpolate vertex normals
		const POLY_CURVE& pc1 = m_curves[crp.ledge.pcind];
		int i1 = pc1.snorm + crp.ledge.cind;
		int i2 = pc1.snorm + (crp.ledge.cind + 1) % pc1.cnt;
		CUnitVector diff = m_norms[i2] - m_norms[i1];
		diff.Scale(crp.ledge.ulen);
		CUnitVector fv1 = m_norms[i1] + diff;

		const POLY_CURVE& pc2 = m_curves[crp.redge.pcind];
		i1 = pc2.snorm + crp.redge.cind;
		i2 = pc2.snorm + (crp.redge.cind + 1) % pc2.cnt;
		diff = m_norms[i2] - m_norms[i1];
		diff.Scale(crp.redge.ulen);
		CUnitVector fv2 = m_norms[i1] + diff;

		double ulen = crp.ledge.dist / (crp.redge.dist + crp.ledge.dist);
		diff = fv2 - fv1;
		diff.Scale(ulen);
		intr.norm = fv1 + diff;
		intr.norm.Normalize();
	}
	else
		intr.norm = crp.norm;

	// insure that the hit normal is pointing in the right direction
	if (!(crp.flags & PCF_CULL) && intr.norm.DotProduct(intr.lray.dir) > 0)
		intr.norm.Invert();
	return CObj::PostIntersect(nthread, intr);
}
