// image.cpp - defines the CImageBuffer class
//

#include <iostream>

#if defined (WIN32) || defined (_WINRT_DLL)
#define WIN_PLATFORM
#else
#undef  WIN_PLATFORM
#endif

#include "image.h"
#include "jpegio.h"
#include "bmpio.h"
#include "pngio.h"
#include "filestructs.h"
#include "migexcept.h"
using namespace std;
using namespace MigRender;

// is this a supported pixel type?
static bool is_valid(ImageFormat fmt)
{
	switch (fmt)
	{
	case ImageFormat::RGBA:
	case ImageFormat::BGRA:
	case ImageFormat::RGB:
	case ImageFormat::BGR:
	case ImageFormat::GreyScale:
	case ImageFormat::Bump:
		return true;
	}
	return false;
}

// returns the size of a single pixel
static int pixel_size(ImageFormat fmt)
{
	switch (fmt)
	{
	case ImageFormat::RGBA: case ImageFormat::BGRA:
		return 4;
	case ImageFormat::RGB: case ImageFormat::BGR:
		return 3;
	case ImageFormat::Bump:
		return 2;
	case ImageFormat::GreyScale:
		return 1;
	}

	return 0;
}

// converts a pixel from one image format to another
static int convert_pixel(const byte* psrc, ImageFormat sfmt, byte* pdst, ImageFormat dfmt)
{
	byte r, g, b, a;
	switch (sfmt)
	{
	case ImageFormat::RGBA: case ImageFormat::RGB:
		r = *psrc++; g = *psrc++; b = *psrc++;
		a = (sfmt == ImageFormat::RGBA ? *psrc : 255);
		break;

	case ImageFormat::BGRA: case ImageFormat::BGR:
		b = *psrc++; g = *psrc++; r = *psrc++;
		a = (sfmt == ImageFormat::RGBA ? *psrc : 255);
		break;

	case ImageFormat::Bump:
		r = *psrc++; g = *psrc;
		b = a = 0;
		break;

	case ImageFormat::GreyScale:
		r = g = b = a = *psrc;
		break;

	default:
		return 0;
	}

	int ret = 0;
	switch (dfmt)
	{
	case ImageFormat::RGBA:	*pdst++ = r; *pdst++ = g; *pdst++ = b; *pdst = a; ret = 4; break;
	case ImageFormat::BGRA:	*pdst++ = b; *pdst++ = g; *pdst++ = r; *pdst = a; ret = 4; break;
	case ImageFormat::RGB:	*pdst++ = r; *pdst++ = g; *pdst = b; ret = 3; break;
	case ImageFormat::BGR:	*pdst++ = b; *pdst++ = g; *pdst = r; ret = 3; break;
	case ImageFormat::Bump:	*pdst++ = r; *pdst = g; ret = 2; break;
	case ImageFormat::GreyScale:	*pdst = (byte) (((int) r + (int) g + (int) b) / 3); ret = 1; break;
	default: return 0;
	}

	return ret;
}

//-----------------------------------------------------------------------------
// CImageBuffer
//-----------------------------------------------------------------------------

CImageBuffer::CImageBuffer(void)
	: m_width(0), m_height(0), m_fmt(ImageFormat::None), m_pxlsize(0)
{
}

CImageBuffer::~CImageBuffer(void)
{
	Delete();
}

void CImageBuffer::Delete(void)
{
	m_data.clear();
	m_width = m_height = 0;
	m_fmt = ImageFormat::None;
}

bool CImageBuffer::Init(int w, int h, ImageFormat fmt)
{
	int pxlsize = pixel_size(fmt);
	if (pxlsize > 0)
	{
		m_data.resize(w*h*pxlsize);
		m_width = w;
		m_height = h;
		m_fmt = fmt;
		m_pxlsize = pxlsize;
	}

	return (pxlsize > 0);
}

void CImageBuffer::SetPath(const char* path)
{
	m_path = path;
}

bool CImageBuffer::WriteLine(int y, byte* pdata, ImageFormat fmt)
{
	if (is_valid(fmt) && y >= 0 && y < m_height)
	{
		byte* pdst = &m_data[y * m_width * m_pxlsize];
		int spxlsize = pixel_size(fmt);
		for (int x = 0; x < m_width; x++)
		{
			pdst += convert_pixel(&pdata[spxlsize*x], fmt, pdst, m_fmt);
		}
		return true;
	}

	return false;
}

bool CImageBuffer::GetSize(int& w, int& h) const
{
	w = m_width;
	h = m_height;

	return true;
}

ImageFormat CImageBuffer::GetFormat(void) const
{
	return m_fmt;
}

const char* CImageBuffer::GetPath(void) const
{
	return m_path.c_str();
}

bool CImageBuffer::GetPixel(int x, int y, ImageFormat fmt, byte* pdata) const
{
	const byte* pbuf = &m_data[(y*m_width + x)*m_pxlsize];
	return (convert_pixel(pbuf, m_fmt, pdata, fmt) > 0);
}

bool CImageBuffer::GetLine(int y, ImageFormat fmt, byte* pdata) const
{
	const byte* pbuf = &m_data[(y*m_width)*m_pxlsize];
	if (fmt == m_fmt)
	{
		// formats match, no conversion necessary
		memcpy(pdata, pbuf, m_width*m_pxlsize);
	}
	else
	{
		for (int x = 0; x < m_width; x++)
		{
			int written = convert_pixel(pbuf, m_fmt, pdata, fmt);
			if (written == 0)
				return false;
			pdata += written;
			pbuf += m_pxlsize;
		}
	}
	return true;
}

const std::vector<byte>& CImageBuffer::GetImageData(void) const
{
	return m_data;
}

bool CImageBuffer::FillAlphaFromTransparentColor(byte r, byte g, byte b, byte dr, byte dg, byte db, byte a)
{
	if (m_fmt != ImageFormat::RGBA && m_fmt != ImageFormat::BGRA)
		return false;

	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width; x++)
		{
			byte* pbuf = &m_data[(y*m_width + x)*m_pxlsize];
			pbuf[3] = 255;

			byte pxl[4];
			convert_pixel(pbuf, m_fmt, pxl, ImageFormat::RGBA);
			if (abs((int) r - pxl[0]) < dr &&
				abs((int) g - pxl[1]) < dg &&
				abs((int) b - pxl[2]) < db)
			{
				pbuf[3] = a;
			}
		}
	}

	return true;
}

bool CImageBuffer::FillAlphaFromImage(const CImageBuffer& img)
{
	if (m_fmt != ImageFormat::RGBA && m_fmt != ImageFormat::BGRA)
		return false;
	if (img.m_width != m_width || img.m_height != m_height)
		return false;

	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width; x++)
		{
			const byte* psrc = &img.m_data[(y*m_width + x)*img.m_pxlsize];
			byte* pdst = &m_data[(y*m_width + x)*m_pxlsize];

			if (img.m_fmt == ImageFormat::GreyScale)
				pdst[3] = *psrc;
			else if (img.m_fmt == ImageFormat::RGB || img.m_fmt == ImageFormat::BGR)
				pdst[3] = (byte) (((int) psrc[0] + (int) psrc[1] + (int) psrc[2] + 1) / 3);
			else if (img.m_fmt == ImageFormat::RGBA || img.m_fmt == ImageFormat::BGRA)
				pdst[3] = psrc[3];
		}
	}

	return true;
}

bool CImageBuffer::FillAlphaFromColors(void)
{
	if (m_fmt != ImageFormat::RGBA && m_fmt != ImageFormat::BGRA)
		return false;

	for (int y = 0; y < m_height; y++)
	{
		for (int x = 0; x < m_width; x++)
		{
			byte* pbuf = &m_data[(y*m_width + x)*m_pxlsize];
			pbuf[3] = (byte) (((int) pbuf[0] + (int) pbuf[1] + (int) pbuf[2] + 1) / 3);
		}
	}

	return true;
}

void CImageBuffer::NormalizeAlpha(void)
{
	if (m_fmt == ImageFormat::RGBA || m_fmt == ImageFormat::BGRA)
	{
		byte min = 255, max = 0;
		for (int y = 0; y < m_height; y++)
		{
			for (int x = 0; x < m_width; x++)
			{
				byte* pbuf = &m_data[(y*m_width + x)*m_pxlsize];
				if (pbuf[3] < min)
					min = pbuf[3];
				if (pbuf[3] > max)
					max = pbuf[3];
			}
		}

		double range = max - min;
		for (int y = 0; y < m_height; y++)
		{
			for (int x = 0; x < m_width; x++)
			{
				byte* pbuf = &m_data[(y*m_width + x)*m_pxlsize];
				pbuf[3] = (byte) (255*((pbuf[3] - min) / range));
			}
		}
	}
}

void CImageBuffer::RenderStart(void)
{
}

bool CImageBuffer::GetTexel(UVC uv, TextureFilter filter, COLOR& col) const
{
	if (uv.u < 0)
		uv.u -= (int) uv.u - 1;
	else if (uv.u >= 1)
		uv.u -= (int) uv.u;
	if (uv.v < 0)
		uv.v -= (int) uv.v - 1;
	else if (uv.v >= 1)
		uv.v -= (int) uv.v;

	// V coords are bottom up, but images are top down
	uv.v = 1 - uv.v;

	byte data[4];
	if (filter == TextureFilter::Nearest)
	{
		double dx = uv.u*(m_width - 1);
		double dy = uv.v*(m_height - 1);
		int x = (int) dx;
		int y = (int) dy;
		if (dx - x > 0.5)
			x++;
		if (dy - y > 0.5)
			y++;

		convert_pixel(&m_data[(y*m_width + x)*m_pxlsize], m_fmt, data, ImageFormat::RGBA);
	}
	else if (filter == TextureFilter::Bilinear)
	{
		double dx = uv.u*(m_width - 1);
		double dy = uv.v*(m_height - 1);
		int x1 = (int) dx;
		int y1 = (int) dy;
		int x2 = (x1 < m_width - 2 ? x1 + 1 : 0);
		int y2 = (y1 < m_height - 2 ? y1 + 1 : 0);
		double xfac = dx - x1;
		double yfac = dy - y1;

		byte dataul[4], dataur[4], datall[4], datalr[4];
		convert_pixel(&m_data[(y1*m_width + x1)*m_pxlsize], m_fmt, dataul, ImageFormat::RGBA);
		convert_pixel(&m_data[(y1*m_width + x2)*m_pxlsize], m_fmt, dataur, ImageFormat::RGBA);
		convert_pixel(&m_data[(y2*m_width + x1)*m_pxlsize], m_fmt, datall, ImageFormat::RGBA);
		convert_pixel(&m_data[(y2*m_width + x2)*m_pxlsize], m_fmt, datalr, ImageFormat::RGBA);

		byte datat[4], datab[4];
		for (int i = 0; i < 4; i++)
		{
			datat[i] = (byte) (dataul[i] + ((int) dataur[i] - (int) dataul[i])*xfac);
			datab[i] = (byte) (datall[i] + ((int) datalr[i] - (int) datall[i])*xfac);
			data[i] = (byte) (datat[i] + ((int) datab[i] - (int) datat[i])*yfac);
		}
	}
	else
		return false;

	col.r = data[0] / 255.0;
	col.g = data[1] / 255.0;
	col.b = data[2] / 255.0;
	col.a = data[3] / 255.0;

	return true;
}

void CImageBuffer::RenderFinish(void)
{
}

//-----------------------------------------------------------------------------
// CImageBufferTarget
//-----------------------------------------------------------------------------

CImageBufferTarget::CImageBufferTarget()
	: CImageBuffer()
{
}

CImageBufferTarget::~CImageBufferTarget()
{
}

REND_INFO CImageBufferTarget::GetRenderInfo(void) const
{
	REND_INFO ri;
	ri.width = (int)m_width;
	ri.height = (int)m_height;
	ri.topdown = false;
	ri.fmt = m_fmt;
	return ri;
}

void CImageBufferTarget::PreRender(int nthreads)
{
	if (m_width == 0 || m_height == 0)
		throw prerender_exception("Width and height must be positive");
}

bool CImageBufferTarget::DoLine(int y, const dword *pline)
{
	return CImageBuffer::WriteLine(y, (byte*) pline, ImageFormat::BGRA);
}

void CImageBufferTarget::PostRender(bool success)
{
}

//-----------------------------------------------------------------------------
// CImageMap
//-----------------------------------------------------------------------------

CImageMap::CImageMap(void)
{
	// add default loaders
	AddLoader(make_unique<CJPEGLoader>());
	AddLoader(make_unique<CPNGLoader>());
#ifdef	WIN_PLATFORM
	AddLoader(make_unique<CBMPLoader>());
#endif	// WIN_PLATFORM
}

CImageMap::~CImageMap(void)
{
	DeleteAll();
}

void CImageMap::AddLoader(std::unique_ptr<CImageLoader> loaderPtr)
{
	m_loaders.push_back(std::move(loaderPtr));
}

string CImageMap::LoadImageFile(const string& path, ImageFormat preffmt, const string& title)
{
	string::size_type pos = path.find_last_of('.');
	if (pos == -1)
		throw fileio_exception("No extension found, cannot load image: " + path);

	string ltitle = title;
	if (ltitle.empty())
	{
		string::size_type pos1 = path.find_last_of('/');
		if (pos1 == -1)
			pos1 = path.find_last_of('\\');
		if (pos1 != -1)
			ltitle = path.substr(pos1+1);
		else
			ltitle = path;
	}

	if (m_map.find(ltitle) != m_map.end())
		return ltitle;

	bool ok = false;
	auto pnew = std::make_unique<CImageBuffer>();
	string ext = path.substr(pos + 1);
	for (const auto& iter : m_loaders)
	{
		if (iter->UsesExtension(ext))
		{
			iter->LoadImage(path.c_str(), pnew.get(), preffmt);
			ok = true;
		}
	}

	if (!ok)
		throw fileio_exception("No handler found for extension: " + ext);

	pnew->SetPath(path.c_str());
	m_map[ltitle] = std::move(pnew);

	// TODO: make sure titles are no longer than 20 characters (including NULL)
	return ltitle;
}

string CImageMap::LoadImageFile(FILE* ifh, CImageLoader& loader, string title, ImageFormat preffmt)
{
	// title string isn't optional here
	if (ifh == NULL || title.length() == 0 || title.length() >= 20)
		throw model_exception("Title string is not optional and cannot be longer than 20 chars");

	if (m_map.find(title) != m_map.end())
		return "";

	auto pnew = std::make_unique<CImageBuffer>();
	loader.LoadImage(ifh, pnew.get(), preffmt);

	m_map[title] = std::move(pnew);
	return title;
}

void CImageMap::DeleteAll(void)
{
	m_map.clear();
}

CImageBuffer* CImageMap::GetImage(const string& title)
{
	return m_map[title].get();
}

const CImageBuffer* CImageMap::GetImage(const string& title) const
{
	const auto& iter = m_map.find(title);
	if (iter == m_map.end())
		return NULL;
	return iter->second.get();
}

void CImageMap::Load(CFileBase& fobj)
{
	FILE_IMAGEMAP fim;
	if (!fobj.ReadNextBlock((byte*) &fim, sizeof(FILE_IMAGEMAP)))
		throw fileio_exception("Error reading FILE_IMAGEMAP");
	LoadImageFile(fim.path, fim.fmt, fim.title);
}

void CImageMap::Save(CFileBase& fobj)
{
	for (const auto& iter : m_map)
	{
		FILE_IMAGEMAP fim;
		if (iter.first.length() >= sizeof(fim.title))
			throw fileio_exception("Image file title too long: " + iter.first);
		strncpy(fim.title, iter.first.c_str(), sizeof(fim.title));
		if (strlen(iter.second->GetPath()) >= sizeof(fim.path))
			throw fileio_exception("Image file name too long: " + string(iter.second->GetPath()));
		strncpy(fim.path, iter.second->GetPath(), sizeof(fim.path));
		fim.fmt = iter.second->GetFormat();
		if (!fobj.WriteDataBlock(BlockType::ImageMap, (byte*) &fim, sizeof(FILE_IMAGEMAP)))
			throw fileio_exception("Error writing image map block");
	}
}
