#pragma once
#include "stdafx.h"
#include "rendertarget.h"

class CRendBitmap :
	public MigRender::CRenderTarget
{
protected:
	HBITMAP m_hbmp;
	BITMAPINFO m_bmi;
	HANDLE m_hMutex;

	LPDWORD m_pbits;
	int m_width;
	int m_height;

	bool m_stop;

protected:
	void DeleteBitmap(void);

public:
	CRendBitmap(void);
	~CRendBitmap(void);

	bool Init(int width, int height, HDC hdc);
	HBITMAP GetBitmap(void);
	int GetWidth(void);
	int GetHeight(void);
	void GetPixel(int x, int y, int& r, int& g, int& b);
	void SetStop(void);
	bool IsStopped(void);

	virtual MigRender::REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);
};
