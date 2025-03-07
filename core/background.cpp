// background.cpp - defines the background handler
//

#include <string>
#include "migexcept.h"
#include "background.h"
using namespace std;
using namespace MigRender;

//-----------------------------------------------------------------------------
// CBackground
//-----------------------------------------------------------------------------

CBackground::CBackground(void)
	: m_pimage(NULL), m_cn(0), m_ce(0), m_cs(0), m_resize(ImageResize::None),
	  m_iwidth(0), m_iheight(0), m_rwidth(0), m_rheight(0), m_iaspect(0), m_raspect(0)
{
}

CBackground::~CBackground(void)
{
}

void CBackground::SetBackgroundColors(const COLOR& cn, const COLOR& ce, const COLOR& cs)
{
	m_cn = cn;
	m_ce = ce;
	m_cs = cs;
}

void CBackground::GetBackgroundColors(COLOR& cn, COLOR& ce, COLOR& cs) const
{
	cn = m_cn;
	ce = m_ce;
	cs = m_cs;
}

void CBackground::SetBackgroundImage(const string& name, ImageResize resize)
{
	m_image = name;
	m_resize = resize;
}

void CBackground::GetBackgroundImage(std::string& name, ImageResize& resize) const
{
	name = m_image;
	resize = m_resize;
}

void CBackground::Load(CFileBase& fobj)
{
	FILE_BACKGROUND fb;
	if (!fobj.ReadNextBlock((byte*)&fb, sizeof(FILE_BACKGROUND)))
		throw fileio_exception("Error reading next block for background");
	m_cn = fb.cn;
	m_ce = fb.ce;
	m_cs = fb.cs;
	m_image = fb.image;
	m_resize = fb.resize;
	m_tm = fb.tm;
	CBaseObj::Load(fobj);
}

void CBackground::Save(CFileBase& fobj)
{
	FILE_BACKGROUND fb;
	fb.cn = m_cn;
	fb.ce = m_ce;
	fb.cs = m_cs;
	if (sizeof(fb.image) <= m_image.length())
		throw fileio_exception("Image path too long saving background");
	strncpy(fb.image, m_image.c_str(), sizeof(fb.image));
	fb.resize = m_resize;
	fb.tm = m_tm;
	if (!fobj.WriteDataBlock(BlockType::Background, (byte*)&fb, sizeof(FILE_BACKGROUND)))
		throw fileio_exception("Error writing data block for background");
	CBaseObj::Save(fobj);
}

void CBackground::PreAnimate(void)
{
	CBaseObj::PreAnimate();

	m_cnOrig = m_cn;
	m_ceOrig = m_ce;
	m_csOrig = m_cs;
}

void CBackground::PostAnimate(void)
{
	CBaseObj::PostAnimate();
}

void CBackground::ResetPlayback()
{
	CBaseObj::ResetPlayback();

	m_cn = m_cnOrig;
	m_ce = m_ceOrig;
	m_cs = m_csOrig;
}

void CBackground::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	switch (animId)
	{
	case AnimType::BgColorNorth:
		m_cn = ComputeNewAnimValue(m_cn, *((COLOR*)newValue), opType);
		break;
	case AnimType::BgColorEquator:
		m_ce = ComputeNewAnimValue(m_ce, *((COLOR*)newValue), opType);
		break;
	case AnimType::BgColorSouth:
		m_cs = ComputeNewAnimValue(m_cs, *((COLOR*)newValue), opType);
		break;
	default:
		CBaseObj::Animate(animId, opType, newValue);
	}
}

void CBackground::RenderStart(CImageMap& images, int w, int h)
{
	m_rwidth = w;
	m_rheight = h;
	m_raspect = m_rwidth / (double) m_rheight;

	if (!m_image.empty())
	{
		m_pimage = images.GetImage(m_image);
		if (m_pimage == NULL)
			throw prerender_exception("Specified image not found: " + m_image);
		m_pimage->GetSize(m_iwidth, m_iheight);
		m_iaspect = m_iwidth / (double) m_iheight;
	}

	m_tm.ComputeInverseTranspose();
}

void CBackground::RenderFinish(void)
{
	m_pimage = NULL;
}

void CBackground::GetRayBackgroundColor(const CRay& ray, COLOR& bg) const
{
	bool usecolors = true;
	if (m_pimage != NULL && ray.pos.IsValid())
	{
		UVC uv;

		if (m_resize == ImageResize::Stretch)
		{
			uv.u = ray.pos.x / (double) m_rwidth;
			uv.v = ray.pos.y / (double) m_rheight;
			usecolors = false;
		}
		else if (m_resize == ImageResize::ScaleToFit)
		{
			if (m_raspect > m_iaspect)
			{
				double hr = m_raspect / m_iaspect;
				uv.u = hr*(ray.pos.x / (double) m_rwidth) - (hr - 1)/2.0;
				uv.v = ray.pos.y / (double) m_rheight;
				usecolors = (uv.u < 0 || uv.u >= 1);
			}
			else
			{
				double hr = m_iaspect / m_raspect;
				uv.u = ray.pos.x / (double) m_rwidth;
				uv.v = hr*ray.pos.y / (double) m_rheight - (hr - 1)/2.0;
				usecolors = (uv.v < 0 || uv.v >= 1);
			}
		}
		else if (m_resize == ImageResize::ScaleToFill)
		{
			if (m_raspect > m_iaspect)
			{
				double hr = m_iaspect / m_raspect;
				uv.u = ray.pos.x / (double) m_rwidth;
				uv.v = (1 - hr)/2.0 + hr*(ray.pos.y) / (double) m_rheight;
			}
			else
			{
				double hr = m_raspect / m_iaspect;
				uv.u = (1 - hr)/2.0 + hr*(ray.pos.x) / (double) m_rwidth;
				uv.v = ray.pos.y / (double) m_rheight;
			}
			usecolors = false;
		}

		if (!usecolors)
			m_pimage->GetTexel(uv, TextureFilter::Bilinear, bg);
	}

	if (usecolors)
	{
		CPt dir = ray.dir;
		m_tm.TransformPoint(dir, 0.0, MatrixType::ICTM);

		bg.r = (dir.y > 0 ? dir.y*m_cn.r + (1.0 - dir.y)*m_ce.r : -dir.y*m_cs.r + (1.0 + dir.y)*m_ce.r);
		bg.g = (dir.y > 0 ? dir.y*m_cn.g + (1.0 - dir.y)*m_ce.g : -dir.y*m_cs.g + (1.0 + dir.y)*m_ce.g);
		bg.b = (dir.y > 0 ? dir.y*m_cn.b + (1.0 - dir.y)*m_ce.b : -dir.y*m_cs.b + (1.0 + dir.y)*m_ce.b);
		bg.a = (dir.y > 0 ? dir.y*m_cn.a + (1.0 - dir.y)*m_ce.a : -dir.y*m_cs.a + (1.0 + dir.y)*m_ce.a);
	}
}
