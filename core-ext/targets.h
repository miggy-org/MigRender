// targets.h - common render target classes
//

#pragma once

#include <vector>

#include "../core/image.h"
#include "../core/rendertarget.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CSplitterTarget - sends render output to multiple targets
//-----------------------------------------------------------------------------

class CSplitterTarget : public CRenderTarget
{
protected:
	std::vector<CRenderTarget*> _targets;

public:

	void AddTarget(CRenderTarget* ptarget);
	void ReplaceTarget(size_t index, CRenderTarget* ptarget);

	// CRenderTarget
	virtual REND_INFO GetRenderInfo(void) const;
	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword* pline);
	virtual void PostRender(bool success);

};

//-----------------------------------------------------------------------------
// CStretchTarget - stretches render output
//-----------------------------------------------------------------------------

class CStretchTarget : public CRenderTarget
{
protected:
	CRenderTarget* _target;
	REND_INFO _srcInfo, _dstInfo;

	double _scaleX, _scaleY;
	std::vector<dword> _lastLine;
	int _lastY;
	double _lastStretchedY;

public:
	CStretchTarget();

	void SetTarget(CRenderTarget* ptarget);
	void SetSourceRendInfo(const REND_INFO& rinfo);

	// CRenderTarget
	virtual REND_INFO GetRenderInfo(void) const;
	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword* pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CBufferedTarget - receives lines into an image buffer, then sends it all out again
//-----------------------------------------------------------------------------

class CBufferedTarget : public CRenderTarget
{
protected:
	CImageBufferTarget _buffer;
	CRenderTarget* _target;

public:
	CBufferedTarget();

	void SetTarget(CRenderTarget* ptarget);

	// CRenderTarget
	virtual REND_INFO GetRenderInfo(void) const;
	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword* pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CPassThroughTarget - pass-through, override this as needed
//-----------------------------------------------------------------------------

class CPassThroughTarget : public CRenderTarget
{
protected:
	CRenderTarget* _target;

public:
	CPassThroughTarget();
	CPassThroughTarget(CRenderTarget* ptarget);

	void SetTarget(CRenderTarget* ptarget);

	// CRenderTarget
	virtual REND_INFO GetRenderInfo(void) const;
	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword* pline);
	virtual void PostRender(bool success);
};

_MIGRENDER_END
