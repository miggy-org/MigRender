// bbox.h - defines bounding boxes
//

#pragma once

#include "vector.h"
#include "ray.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CScreenBox - 2D screen bounding box
//-----------------------------------------------------------------------------

class CScreenBox
{
protected:
	SCREENPOS m_min;
	SCREENPOS m_max;
	bool m_valid;

public:
	CScreenBox(void);
	CScreenBox(const CScreenBox& rhs);
	~CScreenBox(void);

	void StartNewBox(void);
	void AddScreenPoint(const SCREENPOS& pos);
	void FinishNewBox(void);

	bool IsValid(void) const;
	bool IsValid(const SCREENPOS& pos) const;
	bool IsInRect(const SCREENPOS& pos) const;
};

//-----------------------------------------------------------------------------
// CBoundBox - used to easily determine if a ray might hit an object
//
//  During a render if a bounding box is in use then it can be used to easily
//  determine if a ray MIGHT hit an object.  The hit test is a relatively
//  easy one to do, so for complicated objects that don't take up a lot of
//  screen real estate this can dramatically speed up rendering.
//-----------------------------------------------------------------------------

class CBoundBox
{
protected:
	CPt m_minpt;
	CPt m_maxpt;
	bool m_empty;

public:
	CBoundBox(void);
	CBoundBox(const CBoundBox& rhs);
	~CBoundBox(void);

	// used to fill in a bounding box with points - the box will grow dynamically
	//  to enclose all the given points
	void StartNewBox(void);
	void AddPoint(const CPt& pt);
	void AddBox(const CBoundBox& rhs);
	void FinishNewBox(void);

	// box info
	CPt GetCenter(void) const;
	CPt GetMinPt(void) const;
	CPt GetMaxPt(void) const;

	// ray intersection algorithm
	bool IntersectRay(const CRay& ray) const;
};

_MIGRENDER_END
