// bbox.cpp - defines bounding boxes
//

#include <stdlib.h>
#include "bbox.h"
using namespace MigRender;

// determines if x is inside min and max (inclusive)
#define	check_bounds(x, min, max)	(x >= min && x <= max ? true : false)

// swaps two double values
#define	dswap(x, y)					{ double t = x; x = y; y = t; }

//-----------------------------------------------------------------------------
// CScreenBox
//-----------------------------------------------------------------------------

CScreenBox::CScreenBox()
{
}

CScreenBox::CScreenBox(const CScreenBox& rhs)
{
	m_min = rhs.m_min;
	m_max = rhs.m_max;
	m_valid = rhs.m_valid;
}

CScreenBox::~CScreenBox(void)
{
}

void CScreenBox::StartNewBox(void)
{
	m_min.Init();
	m_max.Init();
	m_valid = false;
}

void CScreenBox::AddScreenPoint(const SCREENPOS& pos)
{
	if (pos.IsValid())
	{
		if (!m_min.IsValid())
		{
			m_min = m_max = pos;
		}
		else
		{
			if (pos.x < m_min.x)
				m_min.x = pos.x;
			if (pos.y < m_min.y)
				m_min.y = pos.y;
			if (pos.x > m_max.x)
				m_max.x = pos.x;
			if (pos.y > m_max.y)
				m_max.y = pos.y;
		}
	}
}

void CScreenBox::FinishNewBox(void)
{
	if (m_min.x > 0)
	{
		m_min.x--;
		m_max.x++;
	}
	if (m_min.y > 0)
	{
		m_min.y--;
		m_max.y++;
	}
	m_valid = (m_min.IsValid() && m_max.IsValid());
}

bool CScreenBox::IsValid(void) const
{
	return m_valid;
}

bool CScreenBox::IsValid(const SCREENPOS& pos) const
{
	return (m_valid && pos.IsValid());
}

bool CScreenBox::IsInRect(const SCREENPOS& pos) const
{
	if (!m_valid)
		return true;

	// if the incoming screen position is invalid (probably from a non-camera generated
	//  ray), we actually say it's in the rectangle just to be safe
	if (!pos.IsValid())
		return true;

	if (pos.x >= m_min.x && pos.y >= m_min.y && pos.x <= m_max.x && pos.y <= m_max.y)
		return true;
	return false;
}

//-----------------------------------------------------------------------------
// CBoundBox
//-----------------------------------------------------------------------------

CBoundBox::CBoundBox(void)
	: m_empty(true)
{
}

CBoundBox::CBoundBox(const CBoundBox& rhs)
{
	m_minpt = rhs.m_minpt;
	m_maxpt = rhs.m_maxpt;
	m_empty = rhs.m_empty;
}

CBoundBox::~CBoundBox(void)
{
}

void CBoundBox::StartNewBox(void)
{
	m_empty = true;
}

void CBoundBox::AddPoint(const CPt& pt)
{
	if (m_empty)
	{
		m_minpt = pt;
		m_maxpt = pt;
	}
	else
	{
		if (pt.x < m_minpt.x)
			m_minpt.x = pt.x;
		if (pt.x > m_maxpt.x)
			m_maxpt.x = pt.x;
		if (pt.y < m_minpt.y)
			m_minpt.y = pt.y;
		if (pt.y > m_maxpt.y)
			m_maxpt.y = pt.y;
		if (pt.z < m_minpt.z)
			m_minpt.z = pt.z;
		if (pt.z > m_maxpt.z)
			m_maxpt.z = pt.z;
	}
	m_empty = false;
}

void CBoundBox::AddBox(const CBoundBox& rhs)
{
	AddPoint(rhs.m_minpt);
	AddPoint(rhs.m_maxpt);
}

void CBoundBox::FinishNewBox(void)
{
	if (fabs(m_maxpt.x - m_minpt.x) < EPSILON)
		m_maxpt.x = m_minpt.x + EPSILON;
	if (fabs(m_maxpt.y - m_minpt.y) < EPSILON)
		m_maxpt.y = m_minpt.y + EPSILON;
	if (fabs(m_maxpt.z - m_minpt.z) < EPSILON)
		m_maxpt.z = m_minpt.z + EPSILON;
}

// checks if a ray hits an axis slab
static bool HitSlab(double x, double x0, double dx, double *pft)
{
	// if the direction is parallel to the slab we miss
	if (fabs(dx) < 0.00001)
		return false;

	// otherwise compute the intersection
	*pft = (x - x0) / dx;
	return true;
}

// returns the center of the box
CPt CBoundBox::GetCenter(void) const
{
	return (m_minpt + m_maxpt)/2;
}

// returns the minimum point that defines the box
CPt CBoundBox::GetMinPt(void) const
{
	return m_minpt;
}

// returns the maximum point that defines the box
CPt CBoundBox::GetMaxPt(void) const
{
	return m_maxpt;
}

bool CBoundBox::IntersectRay(const CRay& ray) const
{
	double tmin = -1, tmax = -1, t1, t2;

	// check each of the six slabs and compute the min of the maxs and the max of the mins
	if (HitSlab(m_minpt.x, ray.ori.x, ray.dir.x, &t1))
	{
		HitSlab(m_maxpt.x, ray.ori.x, ray.dir.x, &t2);
		tmin = min(t1, t2); tmax = max(t1, t2);
	}
	else if (!check_bounds(ray.ori.x, m_minpt.x, m_maxpt.x))
		return false;

	if (HitSlab(m_minpt.y, ray.ori.y, ray.dir.y, &t1))
	{
		HitSlab(m_maxpt.y, ray.ori.y, ray.dir.y, &t2);
		if (t1 > t2)
			dswap(t1, t2);
		if (tmin == -1 || t1 > tmin)
			tmin = t1;
		if (tmax == -1 || t2 < tmax)
			tmax = t2;
	}
	else if (!check_bounds(ray.ori.y, m_minpt.y, m_maxpt.y))
		return false;

	if (HitSlab(m_minpt.z, ray.ori.z, ray.dir.z, &t1))
	{
		HitSlab(m_maxpt.z, ray.ori.z, ray.dir.z, &t2);
		if (t1 > t2)
			dswap(t1, t2);
		if (tmin == -1 || t1 > tmin)
			tmin = t1;
		if (tmax == -1 || t2 < tmax)
			tmax = t2;
	}
	else if (!check_bounds(ray.ori.z, m_minpt.z, m_maxpt.z))
		return false;

	// if the min is less than the max we have a hit
	return (tmin <= tmax ? true : false);
}
