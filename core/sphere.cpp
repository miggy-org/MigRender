// sphere.cpp - defines the CSphere class
//

#include "sphere.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CSphere
//-----------------------------------------------------------------------------

CSphere::CSphere(void)
	: m_ori(0, 0, 0), m_rad(0), m_rad2(0)
{
}

CSphere::~CSphere(void)
{
	CObj::RemoveAllMaps();
}

void CSphere::operator=(const CSphere& rhs)
{
	Duplicate(rhs);

	m_ori = rhs.m_ori;
	m_rad = rhs.m_rad;
	m_rad2 = rhs.m_rad2;
}

void CSphere::GetBoundBox(const CMatrix& tm, CBoundBox& bbox) const
{
	double N[3][8];
	N[0][0] = N[0][1] = N[0][2] = N[0][3] = m_ori.x - m_rad;
	N[0][4] = N[0][5] = N[0][6] = N[0][7] = m_ori.x + m_rad;
	N[1][0] = N[1][1] = N[1][4] = N[1][5] = m_ori.y - m_rad;
	N[1][2] = N[1][3] = N[1][6] = N[1][7] = m_ori.y + m_rad;
	N[2][0] = N[2][3] = N[2][4] = N[2][7] = m_ori.z - m_rad;
	N[2][1] = N[2][2] = N[2][5] = N[2][6] = m_ori.z + m_rad;

	bbox.StartNewBox();
	for (int i = 0; i < 8; i++)
	{
		CPt pt(N[0][i], N[1][i], N[2][i]);
		tm.TransformPoint(pt, 1, MatrixType::CTM);
		bbox.AddPoint(pt);
	}
	bbox.FinishNewBox();
}

void CSphere::SetOrigin(const CPt& ori)
{
	m_ori = ori;
}

void CSphere::SetRadius(double rad)
{
	m_rad = rad;
	m_rad2 = rad*rad;
}

void CSphere::Load(CFileBase& fobj)
{
	FILE_SPHERE fs;
	if (!fobj.ReadNextBlock((byte*) &fs, sizeof(FILE_SPHERE)))
		throw fileio_exception("Unable to read sphere");
	m_diff = fs.base.diff;
	m_spec = fs.base.spec;
	m_refl = fs.base.refl;
	m_refr = fs.base.refr;
	m_glow = fs.base.glow;
	m_index = fs.base.index;
	m_flags = fs.base.flags;
	m_tm = fs.base.tm;
	m_filter = fs.base.filter;

	SetOrigin(fs.ori);
	SetRadius(fs.rad);

	CObj::Load(fobj);
}

void CSphere::Save(CFileBase& fobj)
{
	FILE_SPHERE fs;
	fs.base.diff = m_diff;
	fs.base.spec = m_spec;
	fs.base.refl = m_refl;
	fs.base.refr = m_refr;
	fs.base.glow = m_glow;
	fs.base.index = m_index;
	fs.base.flags = m_flags;
	fs.base.tm = m_tm;

	fs.base.filter = m_filter;

	fs.ori = m_ori;
	fs.rad = m_rad;
	if (!fobj.WriteDataBlock(BlockType::Sphere, (byte*) &fs, sizeof(FILE_SPHERE)))
		throw fileio_exception("Unable to write sphere");

	CObj::Save(fobj);
}

void CSphere::RenderStart(int nthreads, const CMatrix& itm, const CCamera& cam, CImageMap& images)
{
	m_lnorms.resize(nthreads);
	CObj::RenderStart(nthreads, itm, cam, images);
}

void CSphere::RenderFinish(void)
{
	m_lnorms.clear();
	CObj::RenderFinish();
}

bool CSphere::IntersectRay(int nthread, const CRay& ray, INTERSECTION& intr)
{
	if (UseBoundBox() && !m_bbox.IntersectRay(ray))
		return false;

	intr.lray = ray;
	double len = m_ftm.TransformRay(intr.lray, MatrixType::ICTM);
	intr.llen = intr.glen = -1;

	CPt ft = m_ori - intr.lray.ori;
	double fl2oc = ft.x*ft.x + ft.y*ft.y + ft.z*ft.z;
	double ftca = ft.x*intr.lray.dir.x + ft.y*intr.lray.dir.y + ft.z*intr.lray.dir.z;

	if (fl2oc < m_rad2)
	{
        double ft2hc = m_rad2 - fl2oc + ftca*ftca;
		intr.llen = ftca + sqrt(ft2hc);
		//pRay->bIn = TRUE;
	}
	else if (ftca > 0)
	{
        double ft2hc = m_rad2 - fl2oc + ftca*ftca;
        if (ft2hc > 0)
		{
            intr.llen = ftca - sqrt(ft2hc);
            //pRay->bIn = FALSE;
		}
	}

	if (intr.llen > 0)
	{
		intr.glen = intr.llen;
		if (len != 1)
			intr.glen *= len;
	}
	return (intr.llen > 0);
}

bool CSphere::ComputeTexelCoord(int nthread, const CTexture* ptxt, UVC& final)
{
	CUnitVector tmp = m_lnorms[nthread];
	tmp.x = -tmp.x;
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
	final.v = (1 + m_lnorms[nthread].y) / 2.0;

	return true;
}

bool CSphere::PostIntersect(int nthread, INTERSECTION& intr)
{
	CPt lhitpt = intr.lray.ori + intr.lray.dir*intr.llen;
	m_lnorms[nthread] = lhitpt - m_ori;
	intr.norm = m_lnorms[nthread];
	intr.norm.Normalize();

	// insure that the hit normal is pointing in the right direction
	if (intr.norm.DotProduct(intr.lray.dir) > 0)
		intr.norm.Invert();
	return CObj::PostIntersect(nthread, intr);
}
