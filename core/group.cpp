// group.cpp - defines the CGrp class
//

#include "group.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CGrp
//-----------------------------------------------------------------------------

CGrp::CGrp(void)
{
}

CGrp::~CGrp(void)
{
	DeleteAll();
}

void CGrp::DeleteAll(void)
{
	m_objs.clear();
	m_grps.clear();
	m_lits.clear();
}

CGrp* CGrp::CreateSubGroup(void)
{
	m_grps.push_back(make_unique<CGrp>());
	return m_grps.back().get();
}

CSphere* CGrp::CreateSphere(void)
{
	m_objs.push_back(make_unique<CSphere>());
	return static_cast<CSphere*>(m_objs.back().get());
}

CPolygon* CGrp::CreatePolygon(void)
{
	m_objs.push_back(make_unique<CPolygon>());
	return static_cast<CPolygon*>(m_objs.back().get());
}

CDirLight* CGrp::CreateDirectionalLight(void)
{
	m_lits.push_back(make_unique<CDirLight>());
	return static_cast<CDirLight*>(m_lits.back().get());
}

CPtLight* CGrp::CreatePointLight(void)
{
	m_lits.push_back(make_unique<CPtLight>());
	return static_cast<CPtLight*>(m_lits.back().get());
}

CSpotLight* CGrp::CreateSpotLight(void)
{
	m_lits.push_back(make_unique<CSpotLight>());
	return static_cast<CSpotLight*>(m_lits.back().get());
}

const CGrpVector& CGrp::GetSubGroups(void) const
{
	return m_grps;
}

const CObjVector& CGrp::GetObjects(void) const
{
	return m_objs;
}

const CLitVector& CGrp::GetLights(void) const
{
	return m_lits;
}

static void ExtractNextAndRemainder(const string& name, string& next, string& rest)
{
	size_t start = 0;
	size_t firstDot = name.find_first_of('.');
	if (firstDot == start)
	{
		start = 1;
		firstDot = name.find_first_of('.', start);
	}
	next = (firstDot != string::npos ? name.substr(start, firstDot - start) : name.substr(start));
	rest = (firstDot != string::npos ? name.substr(firstDot + 1) : "");
}

static size_t ExtractIndexFromName(const string& name)
{
	string indexStr = name.substr(3);
	return atoi(indexStr.c_str());
}

CBaseObj* CGrp::DrillDown(const string& name) const
{
	string next, rest;
	ExtractNextAndRemainder(name, next, rest);

	//for (auto iter = m_grps.begin(); iter != m_grps.end(); iter++)
	for (const auto& iter : m_grps)
	{
		//CGrp* nextgrp = iter.get();
		if (next == iter->GetName())
		{
			if (rest.empty())
				return iter.get();
			return iter->DrillDown(rest);
		}
	}
	//for (auto iter = m_objs.begin(); iter != m_objs.end(); iter++)
	for (const auto& iter : m_objs)
	{
		//CObj* nextobj = iter.get();
		if (next == iter->GetName())
			return iter.get();
	}
	//for (auto iter = m_lits.begin(); iter != m_lits.end(); iter++)
	for (const auto& iter : m_lits)
	{
		//CLight* nextobj = iter.get();
		if (next == iter->GetName())
			return iter.get();
	}

	if (next.find(GroupPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < m_grps.size())
		{
			CGrp* nextgrp = m_grps.at(index).get();
			if (rest.empty())
				return nextgrp;
			return nextgrp->DrillDown(rest);
		}
	}
	else if (next.find(ObjectPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < m_objs.size())
			return m_objs.at(index).get();
	}
	else if (next.find(LightPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < m_lits.size())
			return m_lits.at(index).get();
	}

	return NULL;
}

bool CGrp::ComputeBoundBox(CBoundBox& bbox) const
{
	return ComputeBoundBox(bbox, CMatrix());
}

bool CGrp::ComputeBoundBox(CBoundBox& bbox, const CMatrix& itm) const
{
	CMatrix ftm = m_tm;
	ftm.MatrixMultiply(itm);

	bool ret = false;
	bbox.StartNewBox();

	CBoundBox tmpbox;
	if (!m_objs.empty())
	{
		//for (auto iter = m_objs.begin(); iter != m_objs.end(); iter++)
		for (const auto& iter : m_objs)
		{
			iter->ComputeBoundBox(tmpbox, ftm);
			bbox.AddBox(tmpbox);
			ret = true;
		}
	}

	if (!m_grps.empty())
	{
		//for (auto iter = m_grps.begin(); iter != m_grps.end(); iter++)
		for (const auto& iter : m_grps)
		{
			if (iter->ComputeBoundBox(tmpbox, ftm))
			{
				bbox.AddBox(tmpbox);
				ret = true;
			}
		}
	}

	bbox.FinishNewBox();
	return ret;
}

void CGrp::Load(CFileBase& fobj)
{
	FILE_GROUP fg;
	if (!fobj.ReadNextBlock((byte*) &fg, sizeof(FILE_GROUP)))
		throw fileio_exception("Could not load group block");
	m_tm = fg.tm;

	BlockType bt = fobj.ReadNextBlockType();
	while (bt != BlockType::EndRange)
	{
		if (bt == BlockType::Sphere)
		{
			CSphere* pnew = CreateSphere();
			pnew->Load(fobj);
		}
		else if (bt == BlockType::Polygon)
		{
			CPolygon* pnew = CreatePolygon();
			pnew->Load(fobj);
		}
		else if (bt == BlockType::DirLight)
		{
			CDirLight* pnew = CreateDirectionalLight();
			pnew->Load(fobj);
		}
		else if (bt == BlockType::PtLight)
		{
			CPtLight* pnew = CreatePointLight();
			pnew->Load(fobj);
		}
		else if (bt == BlockType::SpotLight)
		{
			CSpotLight* pnew = CreateSpotLight();
			pnew->Load(fobj);
		}
		else if (bt == BlockType::Group)
		{
			CGrp* pnew = CreateSubGroup();
			pnew->Load(fobj);
		}
		else if (bt != BlockType::EndRange)
			throw fileio_exception("Unrecognized block type while loading group: " + to_string(static_cast<int>(bt)));

		bt = fobj.ReadNextBlockType();
	}

	CBaseObj::Load(fobj);
}

void CGrp::Save(CFileBase& fobj)
{
	FILE_GROUP fg;
	fg.tm = m_tm;
	if (!fobj.WriteDataBlock(BlockType::Group, (byte*) &fg, sizeof(FILE_GROUP)))
		throw fileio_exception("Could not save group block");

	for (const auto& iter : m_objs)
		iter->Save(fobj);

	for (const auto& iter : m_lits)
		iter->Save(fobj);

	for (const auto& iter : m_grps)
		iter->Save(fobj);

	if (!fobj.WriteDataBlock(BlockType::EndRange, NULL, 0))
		throw fileio_exception("Could not save group end of range block");

	CBaseObj::Save(fobj);
}

void CGrp::PreAnimate(void)
{
	CBaseObj::PreAnimate();

	for (const auto& iter : m_grps)
		iter->PreAnimate();
	for (const auto& iter : m_objs)
		iter->PreAnimate();
	for (const auto& iter : m_lits)
		iter->PreAnimate();
}

void CGrp::PostAnimate(void)
{
	CBaseObj::PostAnimate();

	for (const auto& iter : m_grps)
		iter->PostAnimate();
	for (const auto& iter : m_objs)
		iter->PostAnimate();
	for (const auto& iter : m_lits)
		iter->PostAnimate();
}

void CGrp::ResetPlayback(void)
{
	CBaseObj::ResetPlayback();

	for (const auto& iter : m_grps)
		iter->ResetPlayback();
	for (const auto& iter : m_objs)
		iter->ResetPlayback();
	for (const auto& iter : m_lits)
		iter->ResetPlayback();
}

static bool IsAnimObjSpecific(AnimType animId)
{
	return (
		animId == AnimType::ColorDiffuse ||
		animId == AnimType::ColorSpecular ||
		animId == AnimType::ColorReflection ||
		animId == AnimType::ColorRefraction ||
		animId == AnimType::ColorGlow ||
		animId == AnimType::IndexRefraction);
}

static bool IsAnimLitSpecific(AnimType animId)
{
	return (
		animId == AnimType::LightColor ||
		animId == AnimType::LightHLit ||
		animId == AnimType::LightSScale ||
		animId == AnimType::LightFullDistance ||
		animId == AnimType::LightDropDistance);
}

void CGrp::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	bool objSpecific = IsAnimObjSpecific(animId);
	bool litSpecific = IsAnimLitSpecific(animId);

	if (objSpecific || litSpecific)
	{
		// pass the animation down to all sub-groups, in addition to the objects themselves
		for (const auto& iter : m_grps)
			iter->Animate(animId, opType, newValue);
		if (objSpecific)
		{
			for (const auto& iter : m_objs)
				iter->Animate(animId, opType, newValue);
		}
		if (litSpecific)
		{
			for (const auto& iter : m_lits)
				iter->Animate(animId, opType, newValue);
		}
	}
	else
		CBaseObj::Animate(animId, opType, newValue);
}

// collapses the tranformation matricies to the object level, and also fills in a
//  vector with the complete list of all lights
void CGrp::RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images, std::vector<CLight*>& lights)
{
	CMatrix ftm = m_tm;
	ftm.MatrixMultiply(itm);

	for (const auto& iter : m_grps)
		iter->RenderStart(nthreads, ftm, cam, images, lights);

	for (const auto& iter : m_objs)
		iter->RenderStart(nthreads, ftm, cam, images);

	for (const auto& iter : m_lits)
	{
		iter->RenderStart(ftm);
		lights.push_back(iter.get());
	}
}

void CGrp::RenderFinish(void)
{
	for (const auto& iter : m_grps)
		iter->RenderFinish();

	for (const auto& iter : m_objs)
		iter->RenderFinish();

	for (const auto& iter : m_lits)
		iter->RenderFinish();
}

// intersects a ray against all objects and groups within this group
CObj* CGrp::IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr)
{
	INTERSECTION test_intr;
	CObj* pclosest = NULL;
	intr.glen = -1;

	for (const auto& iter : m_grps)
	{
		CObj *ptmp = iter->IntersectRay(nthread, ray, test_intr);
		if (ptmp && (intr.glen == -1 || test_intr.glen < intr.glen))
		{
			pclosest = ptmp;
			intr = test_intr;
		}
	}

	for (const auto& iter : m_objs)
	{
		if (iter->IntersectRay(nthread, ray, test_intr))
		{
			if (intr.glen == -1 || test_intr.glen < intr.glen)
			{
				pclosest = iter.get();
				intr = test_intr;
			}
		}
	}

	return pclosest;
}

// intersects a shadow ray against all objects and groups within this group,
//  returns as soon as a single object is hit (if max == -1 then any object
//  will do, otherwise it needs to be closer than max)
bool CGrp::IntersectShadowRay(int nthread, const CRay& ray, double max)
{
	if (!m_grps.empty())
	{
		INTERSECTION intr;

		for (const auto& iter : m_grps)
		{
			// TODO: why isn't the commented out code the correct version?  won't calling
			//  IntersectRay() possibly mess up temporary rendering vars?
			CObj *ptmp = iter->IntersectRay(nthread, ray, intr);
			if (ptmp && (max == -1 || intr.glen < max))
				return true;
			//if (iter->IntersectShadowRay(nthread, ray, max))
			//	return true;
		}
	}

	if (!m_objs.empty())
	{
		for (const auto& iter : m_objs)
		{
			if (iter->IntersectShadowRay(nthread, ray, max))
				return true;
		}
	}

	return false;
}
