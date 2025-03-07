// polygon.h - defines the polygon class
//

#pragma once

#include "object.h"
#include "image.h"
#include "templates.h"

// polycurve flags
#define PCF_NONE				0x00000000
#define PCF_NEWPLANE			0x00000001	// indicates curve defines a new plane
#define PCF_CULL				0x00000002	// indicates curve should be culled if not facing the viewer
#define PCF_INVISIBLE			0x00000004	// indicates curve is invisible
#define PCF_CW					0x00000008	// indicates curve is clockwise (i.e. a hole)
#define PCF_VERTEXNORMS			0x00000010	// indicates curve uses vertex smoothing

_MIGRENDER_BEGIN

struct POLY_CURVE_REND
{
	// temporary rendering params
	double fd;				// distance to origin
	int discard;			// which coord to discard
	CScreenBox scrbox;		// screen bounding box
	CMatrix tonormtm;		// bump to normal tm

	// temporary extrusion params
	int norigcurve;
	int npoly;
};

struct POLY_CURVE_t
{
	CUnitVector norm;		// normal
	dword flags;			// flags (see PCF_* above)
	int sind;				// index of first index into the lattice array
	int snorm;				// index of first normal in the point normal array
	int smap;				// index of first UV coordinate in any given map array
	int cnt;				// number of indices

	dword reserved;			// reserved
};

struct POLY_CURVE
{
	CUnitVector norm;		// normal
	dword flags;			// flags (see PCF_* above)
	int sind;				// index of first index into the lattice array
	int snorm;				// index of first normal in the point normal array
	int smap;				// index of first UV coordinate in any given map array
	int cnt;				// number of indices

	POLY_CURVE_REND* prp;	// temporary rendering params
};

struct REND_EDGE_HIT
{
	int pcind;				// poly curve index
	int cind;				// corner index
	double ulen;			// length along the line that the hit occurs
	double dist;			// distance from ray-plane intersection point to line
};

struct REND_PARAMS
{
	dword flags;			// flags of the curve that was hit
	double hitlen;			// distance from ray to hit point
	CPt hitpt;				// point on the curve that the ray hit
	CUnitVector norm;		// normal of the curve (doesn't care about vertex shading)
	REND_EDGE_HIT redge;	// used for any sort of edge based interpolation
	REND_EDGE_HIT ledge;	// used for any sort of edge based interpolation
};

//-----------------------------------------------------------------------------
// CPolygon - defines an irregular polygon collection
//-----------------------------------------------------------------------------

class CPolygon : public CObj
{
protected:
	// lattice array
	CLoadArray<CPt> m_lattice;

	// polycurve array
	CLoadArray<POLY_CURVE> m_curves;

	// index array
	CLoadArray<int> m_ind;

	// normals array
	CLoadArray<CUnitVector> m_norms;

	// temporary rendering variables (not thread safe)
	std::vector<REND_PARAMS> m_prp;
	bool m_doedges;

protected:
	std::shared_ptr<CTexture> DupTexture(TextureMapType type, const std::shared_ptr<CTexture>& rhs);

	// smoothing functions (extrude.cpp)
	CUnitVector GetHorizNeighborNorm(int curpoly, int curcurve, int norigcurves, int subpoly, int dir);
	CUnitVector GetVertNeighborNorm(int curpoly, int curcurve, int norigcurves, int subpoly, int dir);
	bool SmoothPolygons(double tol, int norigcurves, int nsegments);

	// extrusion functions (extrude.cpp)
	void AddPolygons(double ang, int ncurve, int npoly,
		const CPt& pt1, const CPt& pt2, const CPt& pt3, const CPt& pt4,
		int iv1, int iv2, int iv3, int iv4);
	CUnitVector ComputeExtrudeVector(const CUnitVector& norm, int sind, int ind, int indcnt, double angle, double len);
	void CleanUpExtrude(void);
	bool Extrude(int fcurve, int ncurves, double ang, double len, bool first);

	// texture rendering
	virtual bool ComputeTexelCoord(int nthread, const CTexture* ptxt, UVC& final);
	virtual bool ComputeBumpMapToNormTM(int nthread, const CUnitVector& norm, CMatrix& tm);

	// bounding box
	virtual void GetBoundBox(const CMatrix& tm, CBoundBox& bbox) const;

	// file I/O
	virtual void LoadMap(BlockType bt, CFileBase& fobj);

public:
	CPolygon(void);
	virtual ~CPolygon(void);

	ObjType GetType(void) const { return ObjType::Polygon; }

	void operator=(const CPolygon& rhs);

	bool LoadLattice(int nlattice, const CPt* pts);
	int LoadLatticePt(const CPt& pt);

	bool AddPolyCurve(const CUnitVector& norm, dword flags, int* indarray, CUnitVector* ptnorms, int nind);
	bool AddPolyCurveIndex(int index);
	bool AddPolyCurveNormal(const CUnitVector& ptnorm);

	int AddColorMap(TextureMapType type, TextureMapOp op, dword flags, const std::string& map, UVC* uvcs = NULL, int num = 0, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	int AddTransparencyMap(dword flags, const std::string& map, UVC* uvcs = NULL, int num = 0, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	int AddBumpMap(dword flags, const std::string& map, double bscale = 1, int btol = 0, UVC* uvcs = NULL, int num = 0, UVC uvMin = UVMIN, UVC uvMax = UVMAX);
	bool AddPolyCurveMapCoord(TextureMapType type, int mindex, const UVC& uvc);

	// defined in texture.cpp
	bool ApplyMapping(TextureMapWrapType wtype, TextureMapType type, int map, bool indc, CPt* pcenter, CUnitVector* paxis, UVC* uvmin = NULL, UVC* uvmax = NULL);

	bool LoadComplete(void);
	void DeleteAll(void);

	const CLoadArray<CPt>& GetLattice(void) const;
	const CLoadArray<POLY_CURVE>& GetCurves(void) const;
	const CLoadArray<int>& GetIndices(void) const;
	const CLoadArray<CUnitVector>& GetNormals(void) const;

	// defined in extrude.cpp
	bool SetClockedness(bool ccw, bool invert);
	bool Extrude(double ang[], double len[], int cnt, bool smooth);
	bool Extrude(double len, bool smooth);
	bool DupExtrudedFacePlate(CPolygon* pnew, bool hideExistingFacePlace);

	// file I/O
	virtual void Load(CFileBase& fobj);
	virtual void Save(CFileBase& fobj);

	virtual void RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images);
	virtual void RenderFinish(void);

	virtual bool IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr);
	virtual bool IntersectShadowRay(int nthread, const CRay& ray, double max);
	virtual bool PostIntersect(int nthread, INTERSECTION& intr);
};

_MIGRENDER_END
