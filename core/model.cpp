// model.cpp - defines the modeling class
//

#include "migexcept.h"
#include "model.h"
using namespace std;
using namespace MigRender;

// this only works for RGBA and BGRA
static dword ConvertPixel(COLOR& col, ImageFormat fmt)
{
	if (col.r < 0) col.r = 0;
	else if (col.r > 1) col.r = 1;
	if (col.g < 0) col.g = 0;
	else if (col.g > 1) col.g = 1;
	if (col.b < 0) col.b = 0;
	else if (col.b > 1) col.b = 1;
	if (col.a < 0) col.a = 0;
	else if (col.a > 1) col.a = 1;

	dword out = 0;
	out |= (dword) (255*col.a);
	out = out << 8;
	if (fmt == ImageFormat::RGBA)
		out |= (dword) (255*col.b);
	else if (fmt == ImageFormat::BGRA)
		out |= (dword) (255*col.r);
	out = out << 8;
	out |= (dword) (255*col.g);
	out = out << 8;
	if (fmt == ImageFormat::RGBA)
		out |= (dword) (255*col.r);
	else if (fmt == ImageFormat::BGRA)
		out |= (dword) (255*col.b);
	return out;
}

struct MigRender::PIXEL_REND_PARAMS
{
	COLOR col;
	CRay ray;
	CPt lookat;
	CObj* phit;
	bool super;
};

//-----------------------------------------------------------------------------
// CModel
//-----------------------------------------------------------------------------

CModel::CModel()
{
	Initialize();
}

CModel::~CModel()
{
	Delete();
}

void CModel::Initialize(void)
{
	Delete();

	m_ptarget = NULL;
	m_ssample = SuperSample::X1;
	m_maxgen = 5;
	m_rendflags = REND_ALL;

	m_cam.GetTM().SetIdentity();

	m_ambient = COLOR(0, 0, 0, 0);
	m_fade = COLOR(0, 0, 0, 0);
	m_fog = COLOR(0, 0, 0, 0);
	m_fogNearDistance = -1;
	m_fogFarDistance = -1;
}

void CModel::Delete(bool includeImages)
{
	m_grp.DeleteAll();
	if (includeImages)
		m_imap.DeleteAll();
}

CBackground& CModel::GetBackgroundHandler(void)
{
	return m_bkgrd;
}

const CBackground& CModel::GetBackgroundHandler(void) const
{
	return m_bkgrd;
}

CCamera& CModel::GetCamera(void)
{
	return m_cam;
}

const CCamera& CModel::GetCamera(void) const
{
	return m_cam;
}

void CModel::SetAmbientLight(const COLOR& ambient)
{
	m_ambient = ambient;
}

const COLOR& CModel::GetAmbientLight(void) const
{
	return m_ambient;
}

void CModel::SetFade(const COLOR& fade)
{
	m_fade = fade;
}

const COLOR& CModel::GetFade(void) const
{
	return m_fade;
}

void CModel::SetFog(const COLOR& fog, double nearDistance, double farDistance)
{
	m_fog = fog;
	m_fogNearDistance = nearDistance;
	m_fogFarDistance = farDistance;
}

const COLOR& CModel::GetFog(void) const
{
	return m_fog;
}

double CModel::GetFogNear(void) const
{
	return m_fogNearDistance;
}

double CModel::GetFogFar(void) const
{
	return m_fogFarDistance;
}

CImageMap& CModel::GetImageMap(void)
{
	return m_imap;
}

const CImageMap& CModel::GetImageMap(void) const
{
	return m_imap;
}

CGrp& CModel::GetSuperGroup(void)
{
	return m_grp;
}

const CGrp& CModel::GetSuperGroup(void) const
{
	return m_grp;
}

void CModel::SetRenderTarget(CRenderTarget* ptarget)
{
	m_ptarget = ptarget;
}

SuperSample CModel::GetSampling(void) const
{
	return m_ssample;
}

void CModel::SetSampling(SuperSample ssample)
{
	m_ssample = ssample;
}

dword CModel::GetRenderQuality(void) const
{
	return m_rendflags;
}

void CModel::SetRenderQuality(dword newrendflags)
{
	m_rendflags = newrendflags;
}

void CModel::Load(CFileBase& fobj)
{
	Delete();

	if (!fobj.OpenFile(false, MigType::Model))
		throw fileio_exception("Unable to open input file object, or invalid file format");

	BlockType bt = fobj.ReadNextBlockType();
	while (bt != BlockType::EndFile)
	{
		if (bt == BlockType::Model)
		{
			FILE_MODEL fm;
			if (!fobj.ReadNextBlock((byte*) &fm, sizeof(FILE_MODEL)))
				throw fileio_exception("Error reading BLOCK_TYPE_MODEL");
			m_ambient = fm.ambient;
		}
		else if (bt == BlockType::Background)
		{
			m_bkgrd.Load(fobj);
		}
		else if (bt == BlockType::Camera)
		{
			m_cam.Load(fobj);
		}
		else if (bt == BlockType::ImageMap)
		{
			m_imap.Load(fobj);
		}
		else if (bt == BlockType::Group)
		{
			m_grp.Load(fobj);
		}
		else if (bt != BlockType::EndFile)
			throw fileio_exception("Unknown block type found: " + to_string(static_cast<int>(bt)));

		bt = fobj.ReadNextBlockType();
	}

	if (!fobj.CloseFile())
		throw fileio_exception("Unable to close input file object");
}

void CModel::Save(CFileBase& fobj)
{
	if (!fobj.OpenFile(true, MigType::Model))
		throw fileio_exception("Unable to open output file object");

	FILE_MODEL fm;
	fm.ambient = m_ambient;
	if (!fobj.WriteDataBlock(BlockType::Model, (byte*) &fm, sizeof(FILE_MODEL)))
		throw fileio_exception("Error writing the model header");

	m_bkgrd.Save(fobj);
	m_cam.Save(fobj);
	m_imap.Save(fobj);
	m_grp.Save(fobj);

	if (!fobj.WriteDataBlock(BlockType::EndFile, NULL, 0))
		throw fileio_exception("Error writing the end-of-file block");

	if (!fobj.CloseFile())
		throw fileio_exception("Unable to close output file object");
}

void CModel::PreAnimate(void)
{
	m_ambientOrig = m_ambient;
	m_fadeOrig = m_fade;
	m_fogOrig = m_fog;
	m_fogNearOrig = m_fogNearDistance;
	m_fogFarOrig = m_fogFarDistance;
}

void CModel::PostAnimate(void)
{
	ResetPlayback();
}

void CModel::ResetPlayback(void)
{
	m_ambient = m_ambientOrig;
	m_fade = m_fadeOrig;
	m_fog = m_fogOrig;
	m_fogNearDistance = m_fogNearOrig;
	m_fogFarDistance = m_fogFarOrig;
}

void CModel::Animate(AnimType animId, AnimOperation opType, const void* newValue)
{
	switch (animId)
	{
	case AnimType::ModelAmbient:
		m_ambient = ComputeNewAnimValue(m_ambient, *((COLOR*)newValue), opType);
		break;
	case AnimType::ModelFade:
		m_fade.a = ComputeNewAnimValue(m_fade.a, *((double*)newValue), opType);
		break;
	case AnimType::ModelFog:
		m_fog.a = ComputeNewAnimValue(m_fog.a, *((double*)newValue), opType);
		break;
	case AnimType::ModelFogNear:
		m_fogNearDistance = ComputeNewAnimValue(m_fogNearDistance, *((double*)newValue), opType);
		break;
	case AnimType::ModelFogFar:
		m_fogFarDistance = ComputeNewAnimValue(m_fogFarDistance, *((double*)newValue), opType);
		break;
	default:
		throw anim_exception("Unsupported animation type: " + to_string(static_cast<int>(animId)));
	}
}

void CModel::RenderStart(int nthreads)
{
	// the camera needs screen dimensions to complete the global-to-screen transformation
	REND_INFO rinfo = m_ptarget->GetRenderInfo();
	m_cam.RenderStart(rinfo.width, rinfo.height);

	// the background handler needs them in case there's a background image
	m_bkgrd.RenderStart(m_imap, rinfo.width, rinfo.height);

	m_lights.clear();
	m_grp.RenderStart(nthreads, CMatrix(), m_cam, m_imap, m_lights);
}

void CModel::RenderFinish(void)
{
	m_lights.clear();
	m_bkgrd.RenderFinish();
	m_grp.RenderFinish();
}

// computes a TM from the XY plane to a given vector (used by soft shadow code below)
//  TODO: this needs some work, when the dir approaces (0,1,0) things get screwy
static void ComputeToDirTM(const CUnitVector& dir, CMatrix& tm)
{
	double rotx = 0, roty = 0;
	CUnitVector tmp;

	tmp.SetPoint(0, dir.y, (dir.z > 0 ? dir.z : -dir.z));
	if (tmp.Normalize() > 0)
		rotx = -atan2(tmp.y, tmp.z);

	tmp.SetPoint(dir.x, 0, dir.z);
	if (tmp.Normalize() > 0)
	{
		roty = atan2(tmp.x, tmp.z);
		if (roty < 0)
			roty += 2*M_PI;
	}

	tm.RotateX(rotx, true);
	tm.RotateY(roty, true);
}

// gets a dithering offset based upon a position in space for the shadow code below
static double GetSoftShadowDither(const CPt& hitpt, double scale)
{
	double r = 10000*(2.34*hitpt.x*hitpt.x + 3.45*hitpt.y*hitpt.y + 4.56*hitpt.z*hitpt.z);
	return scale*(((r - (int) r)/5) - 0.1);
}

// this computes the lighting for a ray that hit an object (doesn't include reflection/refraction rays)
void CModel::ComputeLighting(int nthread, CObj* pobj, const INTERSECTION& intr, COLOR& col)
{
	col.Init();

	// iterate through each available light and compute it's contribution
	for (const auto& plit : m_lights)
	{
		if (plit)
		{
			double litscale = 1;

			// if a light doesn't cast shadows, or of the object doesn't catch shadows
			//  then we don't have a shadow here
			if (plit->IsShadowCaster() && pobj->IsShadowCatcher() && (m_rendflags & REND_AUTO_SHADOWS))
			{
				litscale = 0;

				double dist;
				CRay newray;
				newray.gen = 1;
				newray.ssmpl = m_ssample;
				newray.str.Init();

				CUnitVector dir;
				if (plit->GetDirectionToLight(intr.hit, dir, dist) && dir.DotProduct(intr.norm) > 0)
				{
					if (plit->IsSoftShadow() && (m_rendflags & REND_SOFT_SHADOWS))
					{
						static double offsx[17] = { 1, 0.71, 0, -0.71, -1, -0.71, 0, 0.71, 0.5, 0.35, 0, -0.35, -0.5, -0.35, 0, 0.35, 0 };
						static double offsy[17] = { 0, 0.71, 1, 0.71, 0, -0.71, -1, -0.71, 0, 0.35, 0.5, 0.35, 0, -0.35, -0.5, -0.35, 0 };
						//static double offsx[9] = { 1, 0.71, 0, -0.71, -1, -0.71, 0, 0.71, 0 };
						//static double offsy[9] = { 0, 0.71, 1, 0.71, 0, -0.71, -1, -0.71, 0 };
						static int num = sizeof(offsx) / sizeof(double);
						static double litinc = 1 / (double) num;

						// need to transform the above offsets to the direction vector's space
						CMatrix tm;
						ComputeToDirTM(dir, tm);

						// shoot a series of shadow rays to achieve a (fake) soft effect
						double scale = plit->GetSoftScale() / (10*dist);
						for (int i = 0; i < num; i++)
						{
							CPt offset(scale*offsx[i], scale*offsy[i], 0);
							tm.TransformPoint(offset, 0, MatrixType::CTM);

							newray.ori = intr.hit;
							newray.dir = dir + offset;
							newray.dir.Normalize();
							newray.OffsetOriginByEpsilon();

							if (!m_grp.IntersectShadowRay(nthread, newray, dist))
								litscale += litinc;
						}

						// dither the resulting color
						if (litscale > 0 && litscale < 0.999)
							litscale += GetSoftShadowDither(intr.hit, 1);
						if (litscale < 0)
							litscale = 0;
						else if (litscale > 1)
							litscale = 1;
					}
					else
					{
						newray.ori = intr.hit;
						newray.dir = dir;
						newray.OffsetOriginByEpsilon();

						if (!m_grp.IntersectShadowRay(nthread, newray, dist))
							litscale = 1;
					}
				}
			}

			if (litscale > 0)
			{
				COLOR diffcol, speccol;
				plit->DoLighting(intr.hit, intr.norm, intr.eye, diffcol, speccol);
				diffcol *= (intr.diff*litscale);
				col += diffcol;
				speccol *= (intr.spec*litscale);
				col += speccol;
			}
		}
	}

	// glowing objects don't care about lights
	col += intr.glow;

	// add reflection to the mix (only used for reflection mapping)
	COLOR refl = intr.refl;
	refl *= intr.spec;
	col += refl;

	// finally, add ambient lighting
	COLOR amb = m_ambient;
	amb *= intr.diff;
	col += amb;
}

bool CModel::IsFogEnabled(void)
{
	return (m_fog.a > 0 && m_fogNearDistance > 0 && m_fogFarDistance > 0);
}

void CModel::ApplyFog(COLOR& col, double fogStrength)
{
	COLOR diff = m_fog - col;
	col += diff * (m_fog.a * fogStrength);
}

// this traces a single pixel and returns the color component
CObj* CModel::TracePixel(int nthread, const CRay& ray, COLOR& col)
{
	col.Init();

	// intersect the ray against the supergroup
	INTERSECTION intr;
	CObj* pclosest = m_grp.IntersectRay(nthread, ray, intr);
	if (pclosest != NULL)
	{
		// compute the point where the ray hit something
		intr.hit = ray.ori + ray.dir*intr.glen;

		// compute the vector that points from the hit point to the source of the ray
		intr.eye = ray.dir;
		intr.eye.Invert();

		// setup a copy of the original ray (for reflection mapping)
		intr.gray = ray;

		// post intersection object calculations (normal, colors, etc)
		pclosest->PostIntersect(nthread, intr);

		// compute lighting for that hit point
		COLOR ltcol;
		ComputeLighting(nthread, pclosest, intr, ltcol);
		col += ltcol;

		if (pclosest->IsAutoReflect() && (m_rendflags & REND_AUTO_REFLECT))
		{
			// generate and shoot a reflection ray
			COLOR speccol = intr.spec * ray.str;
			if (speccol.r > 0.01 || speccol.g > 0.01 || speccol.b > 0.01)
			{
				if (ray.gen < m_maxgen)
				{
					CRay newray;
					newray.gen = ray.gen + 1;
					newray.str = intr.spec;
					newray.ori = intr.hit;
					double f2ndoti = 2*intr.norm.DotProduct(ray.dir);
					newray.dir = ray.dir - intr.norm*f2ndoti;
					newray.OffsetOriginByEpsilon();
					newray.ssmpl = m_ssample;
					newray.type = RayType::Reflect;

					TracePixel(nthread, newray, speccol);
					col += speccol;
				}
			}
		}

		col.a = 1;
		if (pclosest->IsAutoRefract() && (m_rendflags & REND_AUTO_REFRACT))
		{
			// generate and shoot a refraction ray
			COLOR refrcol = intr.refr * ray.str;
			if (refrcol.r > 0.01 || refrcol.g > 0.01 || refrcol.b > 0.01)
			{
				if (ray.gen < m_maxgen)
				{
					CRay newray;
					newray.gen = ray.gen + 1;
					newray.str = intr.refr;
					newray.ori = intr.hit;
					double refract = pclosest->GetIndex();
					if (!ray.in)
						newray.in = true;
					else
					{
						refract = 1.0 / refract;
						// since the normal is always corrected in PostIntersect, this may not be necessary
						//intr.norm.Invert();
					}
					CUnitVector v = ray.dir;
					v.Invert();
					double c = 1.0 / fabs(intr.norm.DotProduct(v));
					CUnitVector vp = v;
					vp.Scale(c);
					double length = vp.DotProduct(vp);
					v = intr.norm - vp;
					c = v.DotProduct(v);
					c = refract*refract*length - c;
					if (c > 0)
					{
						c = 1.0 / sqrt(c);
						intr.norm.Scale(c - 1);
						vp.Scale(c);
						newray.dir = intr.norm - vp;
						newray.dir.Normalize();
						newray.OffsetOriginByEpsilon();
						newray.ssmpl = m_ssample;
						newray.type = RayType::Refract;

						TracePixel(nthread, newray, refrcol);
						//col += refrcol;
						if (newray.in)
						{
							col += refrcol;
							col.a = intr.refr.a + (1 - intr.refr.a) * refrcol.a;
						}
						else
						{
							double depthOpacityFactor = 0;

							double refrNear = pclosest->GetRefractionNear();
							double refrFar = pclosest->GetRefractionFar();
							if (refrNear >= 0 && refrFar > refrNear)
							{
								depthOpacityFactor = intr.glen / (refrFar - refrNear);
								if (depthOpacityFactor > 1)
									depthOpacityFactor = 1;
								depthOpacityFactor *= (1 - intr.refr.a);
							}
							col += (refrcol * (1 - depthOpacityFactor));
							col += pclosest->GetGlow() * depthOpacityFactor;
							col.a = refrcol.a;
						}
					}
				}
			}
		}

		if (IsFogEnabled())
		{
			if (intr.glen > m_fogNearDistance)
			{
				double fogStrength = (intr.glen - m_fogNearDistance) / (m_fogFarDistance - m_fogNearDistance);
				ApplyFog(col, fogStrength > 1 ? 1 : fogStrength);
			}
		}
	}
	else
	{
		// the ray didn't hit anything, so get the background color
		if (ray.gen == 1 || ray.type != RayType::Reflect)
		{
			m_bkgrd.GetRayBackgroundColor(ray, col);
			if (IsFogEnabled())
				ApplyFog(col, 1);
		}
	}

	col *= ray.str;
	return pclosest;
}

// super samples a pixel, assumes the center has already been traced, does the surrounding sections
void CModel::SuperSamplePixel(int nthread, PIXEL_REND_PARAMS& curprp, const CPt& deltau, const CPt& deltav)
{
	static double hxdus[8] = { -0.25, 0, 0.25, -0.25, 0.25, -0.25, 0, 0.25 };
	static double hxdvs[8] = { -0.25, -0.25, -0.25, 0, 0, 0.25, 0.25, 0.25 };
	static double lxdus[4] = { -0.25, 0.25, -0.25, 0.25 };
	static double lxdvs[4] = { -0.25, -0.25, 0.25, 0.25 };

	if (m_ssample == SuperSample::X1 || m_ssample == SuperSample::None)
		return;

	double* pdus = (m_ssample == SuperSample::X9 ? hxdus : lxdus);
	double* pdvs = (m_ssample == SuperSample::X9 ? hxdvs : lxdvs);
	int num = (m_ssample == SuperSample::X9 ? 8 : 4);
	for (int i = 0; i < num; i++)
	{
		CRay newray = curprp.ray;
		newray.dir = curprp.lookat + deltau*pdus[i] + deltav*pdvs[i] - curprp.ray.ori;
		newray.dir.Normalize();

		COLOR subcol;
		TracePixel(nthread, newray, subcol);
		curprp.col += subcol; curprp.col.a += subcol.a;
	}
	curprp.col /= (num+1); curprp.col.a /= (num+1);
	curprp.super = true;
}

// returns the next line index to render
//  TODO: not thread safe!!!
int CModel::GetNextLine(void)
{
	int nextLine = m_curr;
	if (m_curr != -1) {
		m_curr += m_dir;
		if (m_curr == m_end)
			m_curr = -1;
	}
	return nextLine;
}

// sends a single scanline of data out to the rendering target
//  TODO: make this thread safe so the target doesn't have to worry about line order
bool CModel::SendLine(PPIXEL_REND_PARAMS pprp, const REND_INFO& rinfo, int y)
{
	vector<dword> line(rinfo.width);
	for (int x = 0; x < rinfo.width; x++)
		line[x] = ConvertPixel(pprp[x].col, rinfo.fmt);

	return m_ptarget->DoLine(y, line.data());
}

// prepares for a render
void CModel::PreRender(int nthreads, CRenderProgress* rprog)
{
	// need a rendering target
	if (!m_ptarget)
		throw prerender_exception("No rendering target supplied");

	// check threading requirements
	m_nthreads = nthreads;
	if (nthreads > 1 && (m_ssample == SuperSample::Edge || m_ssample == SuperSample::Object))
		throw prerender_exception("Cannot do edge super sampling in multi-threaded mode");

	// signal render start
	if (rprog != NULL && !rprog->PreRender(nthreads))
		throw prerender_exception("DoRender() aborted at pre-render stage");

	// signal the start of the render to the rendering target
	m_ptarget->PreRender(nthreads);

	REND_INFO rinfo = m_ptarget->GetRenderInfo();
	m_start = (rinfo.topdown ? rinfo.height - 1 : 0);
	m_end = (rinfo.topdown ? -1 : rinfo.height);
	m_dir = (rinfo.topdown ? -1 : 1);
	m_curr = m_start;

	// signal the start of the render to the supergroup and anything else that needs to know
	RenderStart(nthreads);
}

// this is the main rendering function
bool CModel::DoRender(int nthread, CRenderProgress* rprog)
{
	bool success = true;

	// clear the abort flag
	m_abort = false;

	// get the ray origin and viewing portal from the camera
	CPt ori, vp1, vp2, vp3, vp4;
	m_cam.GetTraceParams(ori, vp1, vp2, vp3, vp4);

	// allocate line buffers
	REND_INFO rinfo = m_ptarget->GetRenderInfo();
	vector<PIXEL_REND_PARAMS> curprp(rinfo.width);
	vector<PIXEL_REND_PARAMS> prvprp(m_ssample == SuperSample::Edge || m_ssample == SuperSample::Object ? rinfo.width : 0);

	// iterate through each pixel
	//int start = (rinfo.topdown ? rinfo.height - 1 : 0);
	//int end = (rinfo.topdown ? -1 : rinfo.height);
	//int dir = (rinfo.topdown ? -1 : 1);
	CPt deltau = (vp2 - vp1) / (double) (rinfo.width - 1);
	CPt deltav = (vp3 - vp1) / (double) (rinfo.height - 1);

	//for (int y = start; y != end && success; y += dir)
	for (int y = GetNextLine(); y != -1 && success; y = GetNextLine())
	{
		//if (m_nthreads > 1 && y%m_nthreads != nthread)
		//	continue;

		for (int x = 0; x < rinfo.width; x++)
		{
			PIXEL_REND_PARAMS& cur = curprp[x];

			// generate a ray for this particular pixel
			cur.lookat = vp1 + deltau*x + deltav*y;
			cur.ray.ori = ori;
			cur.ray.dir = cur.lookat - ori;
			cur.ray.dir.Normalize();
			cur.ray.str.Init(1, 1, 1, 1);
			cur.ray.pos.Init(x, y);
			cur.ray.gen = 1;
			cur.ray.ssmpl = m_ssample;
			cur.super = false;

			// trace the pixel
			cur.phit = TracePixel(nthread, cur.ray, cur.col);

			if (m_ssample == SuperSample::X5 || m_ssample == SuperSample::X9)
			{
				// always super sample these modes
				SuperSamplePixel(nthread, cur, deltau, deltav);
			}
			else if (m_ssample == SuperSample::Edge || m_ssample == SuperSample::Object)
			{
				if (m_ssample == SuperSample::Object)
				{
					// only if the object wants to be super sampled
					if (cur.phit && cur.phit->ShouldSuperSample())
						SuperSamplePixel(nthread, cur, deltau, deltav);
				}

				// check the previous x and y pixels to see if they should be edge sampled
				bool docur = false;
				if (x > 0 && !curprp[x-1].super && curprp[x-1].phit != cur.phit)
				{
					SuperSamplePixel(nthread, curprp[x-1], deltau, deltav);
					docur = true;
				}
				if (y > 0 && !prvprp[x].super && prvprp[x].phit != cur.phit)
				{
					SuperSamplePixel(nthread, prvprp[x], deltau, deltav);
					docur = true;
				}
				if (docur && !cur.super)
					SuperSamplePixel(nthread, cur, deltau, deltav);
			}

			if (m_fade.a > 0)
			{
				COLOR diff = m_fade - cur.col;
				cur.col += diff * m_fade.a;
			}
		}

		if (m_ssample != SuperSample::Edge && m_ssample != SuperSample::Object)
		{
			// send out the current line
			success = SendLine(curprp.data(), rinfo, y);
		}
		else
		{
			// send out the previous line, if any
			if (y > 0)
				success = SendLine(prvprp.data(), rinfo, y - 1);

			// swap lines
			curprp.swap(prvprp);
		}

		if (success && rprog != NULL)
		{
			double percent = 100*(m_dir > 0 ? y / (double) m_end : (m_start - y) / (double) m_start);
			if (!(success = rprog->LineComplete(percent)))
				throw render_exception("DoRender() aborted during render stage");
		}
		if (m_abort)
			success = false;
	}

	// if edge sampled, then we have one last scanline to send out
	if (success && (m_ssample == SuperSample::Edge || m_ssample == SuperSample::Object))
		success = SendLine(curprp.data(), rinfo, rinfo.height - 1);

	return success;
}

// completes a render
void CModel::PostRender(bool success, CRenderProgress* rprog)
{
	// signal the end of the render to the supergroup and anything else that needs to know
	RenderFinish();

	// signal the end of the render to the rendering target
	m_ptarget->PostRender(success);

	// signal render complete
	if (rprog != NULL)
		rprog->PostRender(success);
}

// this is the main rendering function again, handles pre and post render too
bool CModel::DoRenderSimple(CRenderProgress* rprog)
{
	PreRender(1, rprog);
	bool ret = DoRender(0, rprog);
	PostRender(ret, rprog);
	return ret;
}

COLOR CModel::DoRenderPixel(int x, int y)
{
	PreRender(1, NULL);

	// clear the abort flag
	m_abort = false;

	// get the ray origin and viewing portal from the camera
	CPt ori, vp1, vp2, vp3, vp4;
	m_cam.GetTraceParams(ori, vp1, vp2, vp3, vp4);
	REND_INFO rinfo = m_ptarget->GetRenderInfo();
	CPt deltau = (vp2 - vp1) / (double)(rinfo.width - 1);
	CPt deltav = (vp3 - vp1) / (double)(rinfo.height - 1);

	// generate a ray for this particular pixel
	PIXEL_REND_PARAMS cur;
	cur.lookat = vp1 + deltau * x + deltav * y;
	cur.ray.ori = ori;
	cur.ray.dir = cur.lookat - ori;
	cur.ray.dir.Normalize();
	cur.ray.str.Init(1, 1, 1, 1);
	cur.ray.pos.Init(x, y);
	cur.ray.gen = 1;
	cur.ray.ssmpl = SuperSample::X1;
	cur.super = false;

	// trace the pixel
	cur.phit = TracePixel(0, cur.ray, cur.col);

	PostRender(true, NULL);

	return cur.col;
}

// aborts an active render
void CModel::SignalRenderAbort()
{
	m_abort = true;
}
