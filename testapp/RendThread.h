#pragma once
#include "stdafx.h"
#include "rendbitmap.h"

#define WM_REND_START		WM_USER + 1
#define WM_REND_FINISH		WM_USER + 2

struct REND_TREAD_DATA
{
	HWND hdlg;
	HWND hrend;
	BOOL preview;
	MigRender::CRenderTarget* ptarget;
};

extern DWORD WINAPI RendThreadProc(LPVOID lpParam);

extern const TCHAR* RendPixelDebug(REND_TREAD_DATA* pdata, int x, int y);

extern bool ShouldAbortRender(void);
