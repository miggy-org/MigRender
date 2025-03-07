// bmpio.cpp - defines the BMP I/O classes
//

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <windows.h>		// including this causes link errors

#include "bmpio.h"
#include "migexcept.h"
using namespace MigRender;

//
// these are (indirectly) from windows.h
//

typedef unsigned long       DWORD;
typedef unsigned short      WORD;
typedef long                LONG;
typedef unsigned char       BYTE;

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

#include <pshpack2.h>
typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;
#include <poppack.h>

typedef struct tagBITMAPCOREHEADER {
        DWORD   bcSize;
        WORD    bcWidth;
        WORD    bcHeight;
        WORD    bcPlanes;
        WORD    bcBitCount;
} BITMAPCOREHEADER;

typedef struct tagBITMAPINFOHEADER {
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
        BITMAPINFOHEADER    bmiHeader;
        RGBQUAD             bmiColors[1];
} BITMAPINFO;

//-----------------------------------------------------------------------------
// CBMPTarget
//-----------------------------------------------------------------------------

CBMPTarget::CBMPTarget()
	: CRenderTarget(), m_width(0), m_height(0), m_bitcnt(0), m_file(NULL)
{
}

CBMPTarget::~CBMPTarget()
{
}

void CBMPTarget::Init(int width, int height, int bitcnt, const char *path)
{
	m_width = width;
	m_height = height;
	m_bitcnt = bitcnt;
	m_path = path;
}

REND_INFO CBMPTarget::GetRenderInfo(void) const
{
	REND_INFO ri;
	ri.width = (int) m_width;
	ri.height = (int) m_height;
	ri.topdown = false;
	ri.fmt = ImageFormat::BGRA;
	return ri;
}

void CBMPTarget::PreRender(int nthreads)
{
	if (nthreads > 1)
		throw prerender_exception("Image rendering does not support multi-threading yet");
	if (m_width == 0 || m_height == 0)
		throw prerender_exception("Width and height must be non-zero");
	if (m_bitcnt != 24 && m_bitcnt != 32)
		throw prerender_exception("Unsupported bit count: " + std::to_string(m_bitcnt));

	m_file = NULL;
	if (fopen_s(&m_file, m_path.c_str(), "wb"))
		throw fileio_exception("Unable to open file for writing: " + m_path);

	BITMAPFILEHEADER fhdr;
	fhdr.bfType = 0x4d42;
	fhdr.bfReserved1 = fhdr.bfReserved2 = 0;
	fhdr.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	fhdr.bfSize = fhdr.bfOffBits + (m_bitcnt/8)*m_width*m_height;
	if (!fwrite(&fhdr, sizeof(BITMAPFILEHEADER), 1, m_file))
		throw fileio_exception("Unable to write bitmap file header: " + m_path);

	BITMAPINFOHEADER hdr;
	hdr.biSize = sizeof(BITMAPINFOHEADER);
	hdr.biWidth = m_width;
	hdr.biHeight = m_height;
	hdr.biPlanes = 1;
	hdr.biBitCount = m_bitcnt;
	hdr.biCompression = BI_RGB;
	hdr.biSizeImage = 0;
	hdr.biXPelsPerMeter = hdr.biYPelsPerMeter = 2835;
	hdr.biClrUsed = hdr.biClrImportant = 0;
	if (!fwrite(&hdr, sizeof(BITMAPINFOHEADER), 1, m_file))
		throw fileio_exception("Unable to write bitmap info header: " + m_path);
}

bool CBMPTarget::DoLine(int y, const dword *pline)
{
	if (m_bitcnt == 32)
	{
		if (fwrite(pline, 1, 4*m_width, m_file) != 4*m_width)
			throw fileio_exception("Error writing bitmap: " + m_path);
	}
	else
	{
		std::vector<byte> data(3 * m_width);
		for (int x = 0; x < m_width; x++)
		{
			byte *psrc = (byte *) &pline[x];
			byte *pdst = data.data() + 3 * x;
			*pdst++ = *psrc++;
			*pdst++ = *psrc++;
			*pdst++ = *psrc++;
		}
		if (fwrite(data.data(), 1, 3 * m_width, m_file) != 3 * m_width)
			throw fileio_exception("Error writing bitmap: " + m_path);
	}

	return true;
}

void CBMPTarget::PostRender(bool success)
{
	if (m_file != NULL)
		fclose(m_file);
	if (!success)
		remove(m_path.c_str());
}

//-----------------------------------------------------------------------------
// CBMPLoader
//-----------------------------------------------------------------------------

bool CBMPLoader::UsesExtension(const std::string& ext)
{
	return !ext.compare("bmp");
}

void CBMPLoader::LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt)
{
	if (pimg == NULL)
		throw fileio_exception("Image buffer is null: " + std::string(path));

	FILE* fh;
	if (fopen_s(&fh, path, "rb"))
		throw fileio_exception("Unable to open file for reading: " + std::string(path));
	LoadImage(fh, pimg, preffmt);
	fclose(fh);
}

void CBMPLoader::LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt)
{
	if (pimg == NULL)
		throw fileio_exception("Image buffer is null");
	if (ifh == NULL)
		throw fileio_exception("FILE handle is null");

	BITMAPFILEHEADER fhdr;
	if (!fread(&fhdr, sizeof(BITMAPFILEHEADER), 1, ifh))
		throw fileio_exception("Unable to read bitmap file header");
	if (fhdr.bfType != 0x4d42)
		throw fileio_exception("Invalid bitmap file header");

	BITMAPINFOHEADER hdr;
	if (!fread(&hdr, sizeof(BITMAPINFOHEADER), 1, ifh))
		throw fileio_exception("Unable to read bitmap info header");
	if (hdr.biBitCount < 24 || hdr.biCompression != BI_RGB)
		throw fileio_exception("Invalid bit count or compression");

	if (preffmt == ImageFormat::None)
		preffmt = (hdr.biBitCount == 24 ? ImageFormat::RGB : ImageFormat::RGBA);
	int width = hdr.biWidth;
	int height = abs(hdr.biHeight);
	bool topdown = (hdr.biHeight < 0);
	if (!pimg->Init(width, height, preffmt))
		throw fileio_exception("Could not initialize image buffer");

	fseek(ifh, fhdr.bfOffBits, SEEK_SET);

	int elem_size = (hdr.biBitCount == 24 ? 3 : 4);
	int row_stride = width*elem_size;
	std::vector<byte> buffer(row_stride, 0);

	for (int y = 0; y < height; y++)
	{
		if (fread(buffer.data(), elem_size, width, ifh) < (size_t)width)
			throw fileio_exception("Bitmap read failed");
		pimg->WriteLine((topdown ? y : height - y - 1), buffer.data(), (elem_size == 3 ? ImageFormat::BGR : ImageFormat::BGRA));
	}
}
