// model.h - defines the modeling class
//

#pragma once

#include "vector.h"
#include "rendertarget.h"
#include "camera.h"
#include "group.h"
#include "ray.h"
#include "image.h"
#include "background.h"
#include "fileio.h"

// rendering quality flags
#define REND_NONE				0x00000000
#define REND_AUTO_REFLECT		0x00000001
#define REND_AUTO_REFRACT		0x00000002
#define REND_AUTO_SHADOWS		0x00000004
#define REND_SOFT_SHADOWS		0x00000008
#define REND_ALL				0x0000000F

_MIGRENDER_BEGIN

struct PIXEL_REND_PARAMS;
typedef PIXEL_REND_PARAMS* PPIXEL_REND_PARAMS;

// implement this class to receive rendering progress updates, and abort if necessary
class CRenderProgress
{
public:
	virtual bool PreRender(int nthreads) = 0;
	virtual bool LineComplete(double percent) = 0;
	virtual void PostRender(bool success) = 0;
};

//-----------------------------------------------------------------------------
// CModel - holds the entire model
//
//  The basic idea is to instantiate one of these classes, load it up with
//  modeling data, set a rendering target and then call the rendering function
//-----------------------------------------------------------------------------

class CModel : public CAnimTarget
{
protected:
	// the main camera
	CCamera m_cam;

	// background color handler
	CBackground m_bkgrd;

	// ambient color of the background
	COLOR m_ambient;

	// fade
	COLOR m_fade;

	// fog
	COLOR m_fog;
	double m_fogNearDistance;
	double m_fogFarDistance;

	// the image map - contains all loaded images
	CImageMap m_imap;

	// the supergroup - contains the entire model
	CGrp m_grp;

	// the main rendering target
	CRenderTarget* m_ptarget;

	// sampling method
	SuperSample m_ssample;

	// rendering quality flags
	dword m_rendflags;

	// temporary rendering variables
	std::vector<CLight*> m_lights;	// contains all lights during a render
	int m_maxgen;					// maximum recursive rays allowed
	int m_nthreads;					// rendering thread count
	int m_start;					// first rendering line
	int m_end;						// last rendering line
	int m_dir;						// rendering direction
	int m_curr;						// current line
	bool m_abort;					// aborts an active render

	// temporary animation variables
	COLOR m_ambientOrig;
	COLOR m_fadeOrig;
	COLOR m_fogOrig;
	double m_fogNearOrig;
	double m_fogFarOrig;

protected:
	void RenderStart(int nthreads);
	void RenderFinish(void);

	void ComputeLighting(int nthread, CObj* pobj, const INTERSECTION& intr, COLOR& col);
	CObj* TracePixel(int nthread, const CRay& ray, COLOR& col);

	void SuperSamplePixel(int nthread, PIXEL_REND_PARAMS& curprp, const CPt& deltau, const CPt& deltav);

	int GetNextLine(void);
	bool SendLine(PPIXEL_REND_PARAMS pprp, const REND_INFO& rinfo, int y);

	bool IsFogEnabled(void);
	void ApplyFog(COLOR& col, double fogStrength);

public:
	CModel(void);
	virtual ~CModel(void);

	// should be self explanatory
	void Initialize(void);
	void Delete(bool includeImages = true);

	// set the ambient light
	void SetAmbientLight(const COLOR& ambient);
	const COLOR& GetAmbientLight(void) const;

	// fade in/out
	void SetFade(const COLOR& fade);
	const COLOR& GetFade(void) const;

	// fog
	void SetFog(const COLOR& fog, double nearDistance, double farDistance);
	const COLOR& GetFog(void) const;
	double GetFogNear(void) const;
	double GetFogFar(void) const;

	// get the background handler object
	CBackground& GetBackgroundHandler(void);
	const CBackground& GetBackgroundHandler(void) const;

	// get the main camera
	CCamera& GetCamera(void);
	const CCamera& GetCamera(void) const;

	// get the image map
	CImageMap& GetImageMap(void);
	const CImageMap& GetImageMap(void) const;

	// get the supergroup
	CGrp& GetSuperGroup(void);
	const CGrp& GetSuperGroup(void) const;

	// set the rendering target for the DoRender() call
	void SetRenderTarget(CRenderTarget* ptarget);

	// set the super sampling method
	SuperSample GetSampling(void) const;
	void SetSampling(SuperSample ssample);

	// set the rendering quality flag
	dword GetRenderQuality(void) const;
	void SetRenderQuality(dword newrendflags);

	// file I/O
	void Load(CFileBase& fobj);
	void Save(CFileBase& fobj);

	// animation
	void PreAnimate(void);
	void PostAnimate(void);
	void ResetPlayback(void);
	void Animate(AnimType animId, AnimOperation opType, const void* newValue);

	// performs a render (call these 3 in order)
	void PreRender(int nthreads = 1, CRenderProgress* rprog = NULL);
	bool DoRender(int nthread = 0, CRenderProgress* rprog = NULL);
	void PostRender(bool success, CRenderProgress* rprog = NULL);

	// performs a render (simpler version that replaces the 3 above)
	bool DoRenderSimple(CRenderProgress* rprog = NULL);

	// renders a single pixel, used for debugging only
	COLOR DoRenderPixel(int x, int y);

	// aborts an active render
	void SignalRenderAbort();
};

_MIGRENDER_END
