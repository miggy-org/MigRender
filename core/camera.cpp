// camera.cpp - defines a camera
//

#include "camera.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CCamera
//-----------------------------------------------------------------------------

CCamera::CCamera(void)
	: m_dist(10), m_ulen(5), m_vlen(5), m_scrwidth(0), m_scrheight(0), m_distOrig(0), m_ulenOrig(0), m_vlenOrig(0)
{
}

CCamera::CCamera(const CCamera& cam)
	: m_scrwidth(0), m_scrheight(0), m_distOrig(0), m_ulenOrig(0), m_vlenOrig(0)
{
	m_dist = cam.m_dist;
	m_ulen = cam.m_ulen;
	m_vlen = cam.m_vlen;
}

CCamera::~CCamera(void)
{
}

void CCamera::SetViewport(double ulen, double vlen, double dist)
{
	m_ulen = ulen;
	m_vlen = vlen;
	m_dist = dist;
}

void CCamera::GetViewport(double& ulen, double& vlen, double& dist) const
{
	ulen = m_ulen;
	vlen = m_vlen;
	dist = m_dist;
}

void CCamera::Load(CFileBase& fobj)
{
	FILE_CAMERA fc;
	if (!fobj.ReadNextBlock((byte*) &fc, sizeof(FILE_CAMERA)))
		throw fileio_exception("Error reading next block for camera");
	m_dist = fc.dist;
	m_ulen = fc.ulen;
	m_vlen = fc.vlen;
	m_tm = fc.tm;
	CBaseObj::Load(fobj);
}

void CCamera::Save(CFileBase& fobj)
{
	FILE_CAMERA fc;
	fc.dist = m_dist;
	fc.ulen = m_ulen;
	fc.vlen = m_vlen;
	fc.tm = m_tm;
	if (!fobj.WriteDataBlock(BlockType::Camera, (byte*)&fc, sizeof(FILE_CAMERA)))
		throw fileio_exception("Error writing data block for camera");
	CBaseObj::Save(fobj);
}

void CCamera::PreAnimate(void)
{
	CBaseObj::PreAnimate();

	m_distOrig = m_dist;
	m_ulenOrig = m_ulen;
	m_vlenOrig = m_vlen;
}

void CCamera::PostAnimate(void)
{
	CBaseObj::PostAnimate();
}

void CCamera::ResetPlayback(void)
{
	CBaseObj::ResetPlayback();

	m_dist = m_distOrig;
	m_ulen = m_ulenOrig;
	m_vlen = m_vlenOrig;
}

void CCamera::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	double value = *((const double*)newValue);

	switch (animId)
	{
	case AnimType::CameraDist: m_dist = ComputeNewAnimValue(m_dist, value, opType); break;
	case AnimType::CameraULen: m_ulen = ComputeNewAnimValue(m_ulen, value, opType); break;
	case AnimType::CameraVLen: m_vlen = ComputeNewAnimValue(m_vlen, value, opType); break;
	default: CBaseObj::Animate(animId, opType, newValue); break;
	}
}

void CCamera::RenderStart(int w, int h)
{
	m_scrwidth = w;
	m_scrheight = h;

	m_ftm = m_tm;
	m_ftm.ComputeInverseTranspose();
}

void CCamera::RenderFinish(void)
{
}

bool CCamera::GlobalToScreen(const CPt& pt, SCREENPOS& pos) const
{
	// convert the point from global to camera local coords
	CPt lpt = pt;
	m_ftm.TransformPoint(lpt, 1, MatrixType::ICTM);

	// see if the point is behind the camera
	//  NOTE: if a poly is partially behind the camera, this screws up
	if (lpt.z <= 0)
		return false;

	// scale the point to the viewing plane
	double scale = (m_dist / lpt.z);
	lpt.Scale(scale);

	// compute the viewport corners (local coords) and see if the local point is inside
	double xmin = -m_ulen/2;
	double xmax = m_ulen/2;
	double ymin = -m_vlen/2;
	double ymax = m_vlen/2;
	//if (lpt.x < xmin || lpt.x > xmax || lpt.y < ymin || lpt.y > ymax)
	//	return false;

	// interpolate the screen positions
	pos.x = m_scrwidth - (int) (m_scrwidth*(lpt.x - xmin) / m_ulen);
	pos.y = (int) (m_scrheight*(lpt.y - ymin) / m_vlen);
	pos.valid = true;
	return true;
}

bool CCamera::GetTraceParams(CPt& ori, CPt& vp1, CPt& vp2, CPt& vp3, CPt& vp4) const
{
	CPt gpos(0, 0, 0);
	m_tm.TransformPoint(gpos, 1, MatrixType::CTM);
	CUnitVector gdir(0, 0, 1);
	m_tm.TransformPoint(gdir, 0, MatrixType::CTM);

	// compute the view up vector (actually, it's hardcoded for now)
	//  and the view point normal (actually, it's the inverse of the direction vector)
	CUnitVector vuv(0, 1, 0);
	CUnitVector vpn(gdir);
	vpn.Invert();

	// compute the V vector (VUV - VPN*(VUV.VPN))
	double len = vuv.DotProduct(vpn);
	CUnitVector vv = vpn;
	vv.Scale(len);
	vv = vuv - vv;
	vv.Normalize();

	// compute the U vector (V x VPN)
	CUnitVector uv = vv.CrossProduct(vpn);
	uv.Normalize();

	// compute the viewport corners
	vp1 = gpos + gdir*m_dist - uv*m_ulen/2 - vv*m_vlen/2;
	vp2 = gpos + gdir*m_dist + uv*m_ulen/2 - vv*m_vlen/2;
	vp3 = gpos + gdir*m_dist - uv*m_ulen/2 + vv*m_vlen/2;
	vp4 = gpos + gdir*m_dist + uv*m_ulen/2 + vv*m_vlen/2;

	// the origin point is easy
	ori = gpos;

	return true;
}
