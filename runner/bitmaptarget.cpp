#include "bitmaptarget.h"

using namespace MigRender;

CRendBitmap::CRendBitmap(void)
	: m_hbmp(NULL), m_pbits(NULL), m_width(0), m_height(0), m_stop(false)
{
}

CRendBitmap::~CRendBitmap(void)
{
	DeleteBitmap();
}

void CRendBitmap::DeleteBitmap(void)
{
	if (m_hbmp)
	{
		DeleteObject(m_hbmp);
		m_hbmp = NULL;
	}
}

bool CRendBitmap::Init(int width, int height, HDC hdc)
{
	m_width = width;
	m_height = height;

	DeleteBitmap();
	m_hbmp = CreateCompatibleBitmap(hdc, width, height);
	return true;
}

HBITMAP CRendBitmap::GetBitmap(void)
{
	return m_hbmp;
}

int CRendBitmap::GetWidth(void)
{
	return m_width;
}

int CRendBitmap::GetHeight(void)
{
	return m_height;
}

void CRendBitmap::GetPixel(int x, int y, int& r, int& g, int& b)
{
	LPBYTE pdata = new BYTE[4*m_width];

	m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth = m_width;
	m_bmi.bmiHeader.biHeight = m_height;
	m_bmi.bmiHeader.biPlanes = 1;
	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;
	m_bmi.bmiHeader.biSizeImage = 0;
	m_bmi.bmiHeader.biXPelsPerMeter = m_bmi.bmiHeader.biYPelsPerMeter = 0;
	m_bmi.bmiHeader.biClrUsed = m_bmi.bmiHeader.biClrImportant = 0;

	HWND hWnd = GetDesktopWindow();
	HDC hDC = GetDC(hWnd);
	int scan = GetDIBits(hDC, m_hbmp, y, 1, pdata, &m_bmi, DIB_RGB_COLORS);
	ReleaseDC(hWnd, hDC);

	b = pdata[4*x];
	g = pdata[4*x+1];
	r = pdata[4*x+2];

	delete [] pdata;
}

bool CRendBitmap::GetStop(void)
{
	return m_stop;
}

void CRendBitmap::SetStop(bool stop)
{
	m_stop = stop;
}

REND_INFO CRendBitmap::GetRenderInfo(void) const
{
	REND_INFO ri;
	ri.width = m_width;
	ri.height = m_height;
	ri.topdown = false;
	ri.fmt = ImageFormat::BGRA;
	return ri;
}

void CRendBitmap::PreRender(int nthreads)
{
	m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	m_bmi.bmiHeader.biWidth = m_width;
	m_bmi.bmiHeader.biHeight = m_height;
	m_bmi.bmiHeader.biPlanes = 1;
	m_bmi.bmiHeader.biBitCount = 32;
	m_bmi.bmiHeader.biCompression = BI_RGB;
	m_bmi.bmiHeader.biSizeImage = 0;
	m_bmi.bmiHeader.biXPelsPerMeter = m_bmi.bmiHeader.biYPelsPerMeter = 0;
	m_bmi.bmiHeader.biClrUsed = m_bmi.bmiHeader.biClrImportant = 0;

	m_pbits = new DWORD[m_width*m_height];
	memset(m_pbits, 0, m_width*m_height*sizeof(DWORD));

	//SetDIBits(NULL, m_hbmp, 0, m_height, m_pbits, &m_bmi, DIB_RGB_COLORS);
	m_stop = false;
}

bool CRendBitmap::DoLine(int y, const dword* pline)
{
	memcpy(m_pbits + y*m_width, pline, m_width*sizeof(dword));

	const int STEP = 10;
	if (y > 0 && y%STEP == 0)
	{
		int start = y - STEP;
		LPDWORD pbits = &m_pbits[start*m_width];
		SetDIBits(NULL, m_hbmp, start, STEP, pbits, &m_bmi, DIB_RGB_COLORS);
	}

	return (m_stop ? false : true);
}

void CRendBitmap::PostRender(bool success)
{
	int ret = SetDIBits(NULL, m_hbmp, 0, m_height, m_pbits, &m_bmi, DIB_RGB_COLORS);

	delete m_pbits;
	m_pbits = NULL;
}
