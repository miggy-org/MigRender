// extrude.cpp - defines the extrusion routines of CPolygon
//   these routines assume the originating polygons are 2D
//

#include "polygon.h"
using namespace std;
using namespace MigRender;

static POLY_CURVE_REND* MakeTempParams(int norigcurve, int npoly)
{
	POLY_CURVE_REND* prp = new POLY_CURVE_REND;
	prp->norigcurve = norigcurve;
	prp->npoly = npoly;
	return prp;
}

static int GetHWord(dword value)
{
	return (value >> 16);
}

static int GetLWord(dword value)
{
	return (value & 0x0000ffff);
}

//-----------------------------------------------------------------------------
// CPolygon (extrusion routines)
//-----------------------------------------------------------------------------

// simply reverses the clockedness of all the countours
bool CPolygon::SetClockedness(bool ccw, bool invert)
{
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		POLY_CURVE& pc = m_curves[n];

		CUnitVector cross(0, 0, 0);
		for (int i = 0; i < pc.cnt; i++)
		{
			int iv1 = m_ind[pc.sind + i];
			int iv2 = m_ind[pc.sind + (i+1)%pc.cnt];
			int iv3 = m_ind[pc.sind + (i+2)%pc.cnt];

			CUnitVector v1 = m_lattice[iv2] - m_lattice[iv1];
			CUnitVector v2 = m_lattice[iv3] - m_lattice[iv2];
			v1.Normalize();
			v2.Normalize();
			cross = cross + v1.CrossProduct(v2);
		}

		cross.Normalize();
		bool wasccw = (cross.DotProduct(pc.norm) > 0);
		bool isccw = ((wasccw && !invert) || (!wasccw && invert));
		if (isccw)
			pc.flags &= ~PCF_CW;
		else
			pc.flags |= PCF_CW;

		if (invert)
		{
			int start = pc.sind + 1;
			int end = pc.sind + pc.cnt - 1;
			while (end > start)
			{
				int tmp = m_ind[start];
				m_ind[start] = m_ind[end];
				m_ind[end] = tmp;
				start++;
				end--;
			}
		}
	}

	return true;
}

static bool ComputeSmoothNormal(CUnitVector& norm, const CUnitVector& norm1, const CUnitVector& norm2, double tol)
{
	bool ret = false;
	CPt diff(0, 0, 0);

	double dot = norm.DotProduct(norm1);
	if (dot > tol && (1-dot) > EPSILON)
	{
		CPt tmp = norm1 - norm;
		tmp.Scale(0.5);
		diff = diff + tmp;
		ret = true;
	}

	dot = norm.DotProduct(norm2);
	if (dot > tol && (1-dot) > EPSILON)
	{
		CPt tmp = norm2 - norm;
		tmp.Scale(0.5);
		diff = diff + tmp;
		ret = true;
	}

	if (ret)
	{
		norm = norm + diff;
		norm.Normalize();
	}
	return ret;
}

CUnitVector CPolygon::GetHorizNeighborNorm(int curpoly, int curcurve, int norigcurves, int subpoly, int dir)
{
	if (dir > 0 && m_curves[curpoly].cnt == 3)
		curpoly++;
	curpoly += dir;
	if (!m_curves[curpoly].prp || m_curves[curpoly].prp->norigcurve != curcurve)
	{
		do
		{
			curpoly += -dir;
		} while (m_curves[curpoly].prp && m_curves[curpoly].prp->norigcurve == curcurve);
		curpoly += dir;
	}

	if (m_curves[curpoly].cnt == 4)
		return m_curves[curpoly].norm;
	else
	{
		// triangles come in pairs
		if (dir < 0)
			curpoly--;
		CUnitVector ret = m_curves[curpoly].norm + m_curves[curpoly+1].norm;
		ret.Normalize();
		return ret;
	}
}

CUnitVector CPolygon::GetVertNeighborNorm(int curpoly, int curcurve, int norigcurves, int subpoly, int dir)
{
	int first = curcurve;
	int last = m_curves.CountLoaded() - norigcurves + curcurve;
	curpoly += dir;
	if (curpoly > first && curpoly < last)
		curpoly += dir;
	while (curpoly > first && curpoly < last && (!m_curves[curpoly].prp || m_curves[curpoly].prp->npoly != subpoly || m_curves[curpoly].prp->norigcurve != curcurve))
		curpoly += dir;

	if (curpoly == first || curpoly == last)
		return m_curves[curpoly].norm;

	if (m_curves[curpoly].cnt == 4)
		return m_curves[curpoly].norm;
	else
	{
		// triangles come in pairs
		if (dir < 0)
			curpoly--;
		CUnitVector ret = m_curves[curpoly].norm + m_curves[curpoly+1].norm;
		ret.Normalize();
		return ret;
	}
}

// please note that this only works on extruded polygons
bool CPolygon::SmoothPolygons(double tol, int norigcurves, int nsegments)
{
	int poly = norigcurves;
	while (poly < m_curves.CountLoaded() - norigcurves)
	{
		POLY_CURVE& pc = m_curves[poly++];
		if (!(pc.flags & PCF_INVISIBLE))
		{
			int norigcurve = pc.prp->norigcurve;
			int npoly = pc.prp->npoly;

			CUnitVector lasth = GetHorizNeighborNorm(poly-1, norigcurve, norigcurves, npoly, -1);
			CUnitVector nexth = GetHorizNeighborNorm(poly-1, norigcurve, norigcurves, npoly, 1);
			CUnitVector lastv = GetVertNeighborNorm(poly-1, norigcurve, norigcurves, npoly, -1);
			CUnitVector nextv = GetVertNeighborNorm(poly-1, norigcurve, norigcurves, npoly, 1);

			CUnitVector norm = pc.norm;
			if (pc.cnt == 3)
			{
				CUnitVector norm1 = m_curves[poly].norm;
				norm = norm + norm1;
				norm.Normalize();
			}

			bool dosmooth = false;
			CUnitVector n1 = norm;
			if (ComputeSmoothNormal(n1, lasth, lastv, tol))
				dosmooth = true;
			CUnitVector n2 = norm;
			if (ComputeSmoothNormal(n2, nexth, lastv, tol))
				dosmooth = true;
			CUnitVector n3 = norm;
			if (ComputeSmoothNormal(n3, nexth, nextv, tol))
				dosmooth = true;
			CUnitVector n4 = norm;
			if (ComputeSmoothNormal(n4, lasth, nextv, tol))
				dosmooth = true;

			if (dosmooth)
			{
				pc.flags |= PCF_VERTEXNORMS;
				pc.snorm = m_norms.Count();

				if (pc.cnt == 4)
				{
					AddPolyCurveNormal(n1);
					AddPolyCurveNormal(n2);
					AddPolyCurveNormal(n3);
					AddPolyCurveNormal(n4);
				}
				else if (pc.cnt == 3)
				{
					AddPolyCurveNormal(n1);
					AddPolyCurveNormal(n2);
					AddPolyCurveNormal(n3);

					POLY_CURVE& pc2 = m_curves[poly++];
					pc2.flags |= PCF_VERTEXNORMS;
					pc2.snorm = m_norms.Count();
					AddPolyCurveNormal(n3);
					AddPolyCurveNormal(n4);
					AddPolyCurveNormal(n1);
				}
			}
			else if (pc.cnt == 3)
				poly++;
		}
	}

	return LoadComplete();
}

CUnitVector CPolygon::ComputeExtrudeVector(const CUnitVector& norm, int sind, int ind, int indcnt, double angle, double len)
{
	int i1 = m_ind[sind + (ind == 0 ? indcnt - 1 : ind - 1)];
	int i2 = m_ind[sind + ind];
	int i3 = m_ind[sind + (ind + 1)%indcnt];

	CPt pt1 = m_lattice[i1];
	CPt pt2 = m_lattice[i2];
	CPt pt3 = m_lattice[i3];

	CUnitVector v1 = pt1 - pt2;
	v1.Normalize();
	CUnitVector v2 = pt3 - pt2;
	v2.Normalize();

	CUnitVector inner = v1 + v2;
	double ilen = inner.Normalize();

	CUnitVector check = v1.CrossProduct(norm);
	if (ilen > 0)
	{
		if (check.DotProduct(inner) < 0)
			inner.Invert();
	}
	else
	{
		inner = check;
	}

	// since M_PI isn't exactly equal to PI, I fudge in 90 degrees manually
	double nfac = len, ifac = 0;
	if (angle != 90)
	{
		angle = M_PI*angle / 180.0;
		nfac = (angle == 90 ? 1 : sin(angle))*len;
		ifac = (angle == 90 ? 0 : cos(angle))*len;
	}

	// find the angle between v1 and inner, and scale inner by the inverse of the sin
	// TODO: what is the point of this?
	//if (ifac > 0)
	//{
	//	ilen = fabs(inner.DotProduct(v1));
	//	if (ilen < 0.99)
	//		ifac /= sin(acos(ilen));
	//}

	CUnitVector ret = (norm*nfac + inner*ifac);
	//ret.Normalize();
	return ret;
}

// TODO: shouldn't new bevel polygons always be planar?
static bool ArePlanar(const CPt& pt1, const CPt& pt2, const CPt& pt3, const CPt& pt4)
{
	CUnitVector v1 = pt2 - pt1;
	CUnitVector v2 = pt3 - pt2;
	CUnitVector norm1 = v1.CrossProduct(v2);

	v1 = pt4 - pt3;
	CUnitVector norm2 = v2.CrossProduct(v1);

	return (norm1 == norm2);
}

void CPolygon::AddPolygons(double ang, int ncurve, int npoly,
						   const CPt& pt1, const CPt& pt2, const CPt& pt3, const CPt& pt4,
						   int iv1, int iv2, int iv3, int iv4)
{
	CUnitVector v1, v2, norm;

	if (ang == 0 || ang == 90 || ang == 180 || ArePlanar(pt1, pt2, pt3, pt4))
	{
		v1 = pt2 - pt1;
		v2 = pt3 - pt2;
		norm = v1.CrossProduct(v2);
		norm.Normalize();

		AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
		AddPolyCurveIndex(iv1);
		AddPolyCurveIndex(iv2);
		AddPolyCurveIndex(iv3);
		AddPolyCurveIndex(iv4);

		// mark the new curve with original curve index and it's sequence number
		//  (for the smoothing algorithm)
		int nc = m_curves.Count();
		POLY_CURVE& pc = m_curves[nc-1];
		pc.prp = MakeTempParams(ncurve, npoly);
	}
	else
	{
		v1 = pt2 - pt1;
		v2 = pt3 - pt2;
		norm = v1.CrossProduct(v2);
		norm.Normalize();

		AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
		AddPolyCurveIndex(iv1);
		AddPolyCurveIndex(iv2);
		AddPolyCurveIndex(iv3);

		v1 = pt4 - pt3;
		v2 = pt1 - pt4;
		norm = v1.CrossProduct(v2);
		norm.Normalize();

		AddPolyCurve(norm, PCF_NEWPLANE | PCF_CULL, NULL, NULL, 0);
		AddPolyCurveIndex(iv3);
		AddPolyCurveIndex(iv4);
		AddPolyCurveIndex(iv1);

		// mark the new curves with original curve index and it's sequence number
		//  (for the smoothing algorithm)
		int nc = m_curves.Count();
		POLY_CURVE& pc1 = m_curves[nc-1];
		pc1.prp = MakeTempParams(ncurve, npoly);
		POLY_CURVE& pc2 = m_curves[nc-2];
		pc2.prp = MakeTempParams(ncurve, npoly);
	}
}

void CPolygon::CleanUpExtrude(void)
{
	for (int n = 0; n < m_curves.CountLoaded(); n++)
	{
		POLY_CURVE& pc = m_curves[n];
		if (pc.prp != NULL)
		{
			delete pc.prp;
			pc.prp = NULL;
		}
	}
}

// takes all curves and performs a single extrusion
//  this code assumes that all holes have the opposite clockedness
bool CPolygon::Extrude(int fcurve, int ncurves, double ang, double len, bool first)
{
	vector<int> newind;
	for (int n = 0; n < ncurves; n++)
	{
		const POLY_CURVE& pc = m_curves[fcurve+n];

		int iv1 = m_ind[pc.sind];
		int iv2 = m_ind[pc.sind+1];

		CUnitVector ext1 = ComputeExtrudeVector(pc.norm, pc.sind, 0, pc.cnt, ang, len);
		CUnitVector ext2 = ComputeExtrudeVector(pc.norm, pc.sind, 1, pc.cnt, ang, len);

		CPt pt1 = m_lattice[iv1];
		CPt pt2 = m_lattice[iv2];
		CPt pt3 = pt2 + ext2;
		CPt pt4 = pt1 + ext1;
		int iv4 = LoadLatticePt(pt4);
		int iv3 = LoadLatticePt(pt3);

		newind.push_back(iv4);
		newind.push_back(iv3);

		// this is so we don't add the last point twice
		int firstind = iv4;
		CPt firstpt = pt4;

		AddPolygons(ang, n, 0, pt1, pt2, pt3, pt4, iv1, iv2, iv3, iv4);

		iv1 = iv2;
		pt1 = pt2;
		iv4 = iv3;
		pt4 = pt3;
		for (int i = 1; i < pc.cnt; i++)
		{
			iv2 = m_ind[pc.sind+(i+1)%pc.cnt];
			pt2 = m_lattice[iv2];
			if (i == pc.cnt - 1)
			{
				iv3 = firstind;
				pt3 = firstpt;
			}
			else
			{
				ext2 = ComputeExtrudeVector(pc.norm, pc.sind, i+1, pc.cnt, ang, len);
				pt3 = pt2 + ext2;
				iv3 = LoadLatticePt(pt3);
				newind.push_back(iv3);
			}

			AddPolygons(ang, n, i, pt1, pt2, pt3, pt4, iv1, iv2, iv3, iv4);

			iv1 = iv2;
			pt1 = pt2;
			iv4 = iv3;
			pt4 = pt3;

			// TODO: should ext2 be reassigned to ext1?
			//ext1 = ext2;
		}
	}

	int newcnt = 0;
	for (int n = 0; n < ncurves; n++)
	{
		POLY_CURVE& pc = m_curves[fcurve+n];
		pc.norm.Invert();
		if (!(m_flags & OBJF_REFR_RAY))
			pc.flags |= PCF_CULL;

		CUnitVector norm = pc.norm;
		norm.Invert();

		AddPolyCurve(norm, pc.flags, NULL, NULL, 0);
		for (int i = 0; i < pc.cnt; i++)
		{
			int tmp = newind[newcnt++];
			AddPolyCurveIndex(tmp);
		}

		if (!first)
			pc.flags |= PCF_INVISIBLE;
	}

	return LoadComplete();
}

// takes all curves and performs a series of extrusions
//  this code assumes that all holes have the opposite clockedness
bool CPolygon::Extrude(double ang[], double len[], int cnt, bool smooth)
{
	int fcurve = 0;
	int ncurves = m_curves.CountLoaded();
	for (int n = 0; n < cnt; n++)
	{
		if (!Extrude(fcurve, ncurves, ang[n], len[n], (n == 0)))
			return false;
		fcurve = m_curves.CountLoaded() - ncurves;
	}

	bool ret = (smooth ? SmoothPolygons(0.9, ncurves, cnt) : true);
	CleanUpExtrude();
	return ret;
}

// simple perpendicular extrusion
bool CPolygon::Extrude(double len, bool smooth)
{
	int ncurves = m_curves.CountLoaded();
	if (!Extrude(0, m_curves.CountLoaded(), 90, len, true))
		return false;

	bool ret = (smooth ? SmoothPolygons(0.9, ncurves, 1) : true);
	CleanUpExtrude();
	return ret;
}

// duplicates the face plate from a previous extrusion
bool CPolygon::DupExtrudedFacePlate(CPolygon* pnew, bool hideExistingFacePlace)
{
	// find out how many curves the faceplate has
	//  this is kinda kludgy, we'll see if i actually need to fix this later
	int ncurves = m_curves.CountLoaded();
	if (ncurves > 0)
	{
		int ncnt = 1;
		for (int n = 1; n < ncurves; n++)
		{
			const POLY_CURVE& pc = m_curves[n];
			//if (pc.cnt <= 4 && !(pc.flags & PCF_CW))
			if (pc.cnt <= 4 && !(pc.flags & PCF_CW) && (pc.flags & PCF_NEWPLANE))
				break;
			ncnt++;
		}

		// duplicate each poly curve
		for (int n = ncurves - ncnt; n < ncurves; n++)
		{
			POLY_CURVE& pc = m_curves[n];

			pnew->AddPolyCurve(pc.norm, pc.flags, NULL, NULL, 0);
			for (int i = 0; i < pc.cnt; i++)
			{
				int npt = pnew->LoadLatticePt(m_lattice[m_ind[pc.sind + i]]);
				pnew->AddPolyCurveIndex(npt);
			}

			// set the original to invisible
			if (hideExistingFacePlace)
				pc.flags |= PCF_INVISIBLE;
		}
	}
	pnew->LoadComplete();

	// copy color attributes but not textures
	pnew->m_diff = m_diff;
	pnew->m_spec = m_spec;
	pnew->m_refl = m_refl;
	pnew->m_refr = m_refr;
	pnew->m_glow = m_glow;
	pnew->m_index = m_index;
	pnew->m_flags = m_flags;
	pnew->m_tm = m_tm;

	return true;
}
