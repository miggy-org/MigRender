// background.h - defines the background handler
//

#pragma once

#include "image.h"
#include "matrix.h"
#include "ray.h"
#include "fileio.h"
#include "baseobject.h"

_MIGRENDER_BEGIN

class CBackground : public CBaseObj
{
protected:
	// colors (north pole, equator, south pole)
	COLOR m_cn;
	COLOR m_ce;
	COLOR m_cs;

	// image
	std::string m_image;
	ImageResize m_resize;

	// temporary rendering vars (thread safe)
	CImageBuffer* m_pimage;
	int m_iwidth, m_iheight;
	int m_rwidth, m_rheight;
	double m_iaspect, m_raspect;

	// temporary animation vars
	COLOR m_cnOrig;
	COLOR m_ceOrig;
	COLOR m_csOrig;

public:
	CBackground(void);
	~CBackground(void);

	ObjType GetType(void) const { return ObjType::Bg; }

	void SetBackgroundColors(const COLOR& cn, const COLOR& ce, const COLOR& cs);
	void GetBackgroundColors(COLOR& cn, COLOR& ce, COLOR& cs) const;
	void SetBackgroundImage(const std::string& name, ImageResize resize);
	void GetBackgroundImage(std::string& name, ImageResize& resize) const;

	// file I/O
	void Load(CFileBase& fobj);
	void Save(CFileBase& fobj);

	// animation
	void PreAnimate(void);
	void PostAnimate(void);
	void ResetPlayback(void);
	void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	void RenderStart(CImageMap& images, int w, int h);
	void RenderFinish(void);

	void GetRayBackgroundColor(const CRay& ray, COLOR& bg) const;
};

_MIGRENDER_END
