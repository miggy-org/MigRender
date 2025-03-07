// rendertarget.h - defines a rendering target class
//

#pragma once

#include "defines.h"

_MIGRENDER_BEGIN

// describes the rendering targets preferred image format
struct REND_INFO
{
	int width;
	int height;
	bool topdown;
	ImageFormat fmt;
};

//-----------------------------------------------------------------------------
// CRenderTarget - rendering target base class
//
//  This is an abstract class - derived classes can be used as rendering
//  targets by the modeling class.
//-----------------------------------------------------------------------------

class CRenderTarget
{
public:
	virtual REND_INFO GetRenderInfo(void) const = 0;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline) = 0;
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CRenderTargetThreadSafe - helper class to assist with rendering targets that aren't thread-safe
//
//  Not complete, missing a portable mutex implementation.
//-----------------------------------------------------------------------------

/*class CRenderTargetThreadSafe
{
private:
	CRenderTarget* m_pTarget;
	bool m_isThreadSafe;
	void* m_pLineMap;
	int m_nextLine;
	int m_lastLine;
	int m_dir;

private:
	bool FlushLines();
	void FlushAllLines();

public:
	CRenderTargetThreadSafe();

	void SetTarget(CRenderTarget* ptarget);
	void PreRender(int nthreads);
	bool DoLine(int y, const dword *pline);
	void PostRender(void);
};*/

_MIGRENDER_END
