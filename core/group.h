// group.h - defines grouping
//

#pragma once

#include "object.h"
#include "lights.h"
#include "sphere.h"
#include "polygon.h"
#include "camera.h"
#include "fileio.h"
#include "baseobject.h"

_MIGRENDER_BEGIN

class CGrp;

typedef std::vector<std::unique_ptr<CGrp>> CGrpVector;
typedef std::vector<std::unique_ptr<CObj>> CObjVector;
typedef std::vector<std::unique_ptr<CLight>> CLitVector;

const std::string GroupPrefixName = "grp";
const std::string ObjectPrefixName = "obj";
const std::string LightPrefixName = "lit";

//-----------------------------------------------------------------------------
// CGrp - a grouping class
//
//  This class holds a list of objects, lights, and sub-groups.  It also
//  has it's own transformation matrix.
//-----------------------------------------------------------------------------

class CGrp : public CBaseObj
{
protected:
	CGrpVector m_grps;
	CObjVector m_objs;
	CLitVector m_lits;

public:
	CGrp(void);
	~CGrp(void);

	ObjType GetType(void) const { return ObjType::Group; }

	void DeleteAll(void);

	CGrp* CreateSubGroup(void);
	CSphere* CreateSphere(void);
	CPolygon* CreatePolygon(void);

	CDirLight* CreateDirectionalLight(void);
	CPtLight* CreatePointLight(void);
	CSpotLight* CreateSpotLight(void);

	// for iteration only, do not create/insert new objects using these
	const CGrpVector& GetSubGroups(void) const;
	const CObjVector& GetObjects(void) const;
	const CLitVector& GetLights(void) const;

	// drills down into the group heirarchy using a namespace string
	CBaseObj* DrillDown(const std::string& name) const;

	bool ComputeBoundBox(CBoundBox& bbox) const;
	bool ComputeBoundBox(CBoundBox& bbox, const CMatrix& itm) const;

	// file I/O
	void Load(CFileBase& fobj);
	void Save(CFileBase& fobj);

	// animation
	void PreAnimate(void);
	void PostAnimate(void);
	void ResetPlayback(void);
	void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	void RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images, std::vector<CLight*>& lights);
	void RenderFinish(void);

	CObj* IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr);
	bool IntersectShadowRay(int nthread, const CRay& ray, double max);
};

_MIGRENDER_END
