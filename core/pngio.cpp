// pngio.cpp - defines the PNG I/O classes
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
// TODO: produce a POSIX compatible libpng
#if    __ANDROID__
#include "../lib/libpng-android/png.h"
#else
#include "../lib/libpng/png.h"
#endif // __ANDROID__
}

#include "pngio.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CPNGTarget
//-----------------------------------------------------------------------------

CPNGTarget::CPNGTarget()
	: CRenderTarget(), m_width(0), m_height(0), m_file(NULL), m_info(NULL), m_pngptr(NULL)
{
}

CPNGTarget::~CPNGTarget()
{
	PNGDestroy();
}

void CPNGTarget::PNGDestroy(void)
{
	if (m_pngptr != NULL)
	{
		png_destroy_write_struct((png_structpp) &m_pngptr, (png_infopp) &m_info);
		m_pngptr = NULL;
		m_info = NULL;
	}
}

void CPNGTarget::Init(int width, int height, const char *path)
{
	m_width = width;
	m_height = height;
	m_path = path;
}

REND_INFO CPNGTarget::GetRenderInfo(void) const
{
	REND_INFO ri;
	ri.width = m_width;
	ri.height = m_height;
	ri.topdown = true;
	ri.fmt = ImageFormat::RGBA;
	return ri;
}

void CPNGTarget::PreRender(int nthreads)
{
	if (nthreads > 1)
		throw prerender_exception("Image rendering does not support multi-threading yet");
	if (m_width == 0 || m_height == 0)
		throw prerender_exception("Width and height must be non-zero");

	m_pngptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);
	m_info = png_create_info_struct((png_structp) m_pngptr);
	if (m_pngptr == NULL || m_info == NULL)
		throw fileio_exception("Unable to initialize PNG: " + m_path);

	if (setjmp(png_jmpbuf((png_structp) m_pngptr)))
	{
		PNGDestroy();
		throw fileio_exception("Unable to initialize PNG: " + m_path);
	}
	if ((m_file = fopen(m_path.c_str(), "wb")) == NULL)
	{
		PNGDestroy();
		throw fileio_exception("Unable to initialize PNG: " + m_path);
	}

	png_init_io((png_structp) m_pngptr, m_file);

	// set the header information
	png_set_IHDR((png_structp) m_pngptr, (png_infop) m_info,
		m_width, m_height, 8,
		PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);

	// future - consider adding text comments to file

	// write the file header
	png_write_info((png_structp) m_pngptr, (png_infop) m_info);
}

bool CPNGTarget::DoLine(int y, const dword *pline)
{
	png_bytep rowp = (png_bytep) pline;
	png_write_rows((png_structp) m_pngptr, &rowp, 1);

	return true;
}

void CPNGTarget::PostRender(bool success)
{
	png_write_end((png_structp) m_pngptr, (png_infop) m_info);

	PNGDestroy();
	fclose(m_file);
	if (!success)
		remove(m_path.c_str());
}

//-----------------------------------------------------------------------------
// CPNGLoader
//-----------------------------------------------------------------------------

bool CPNGLoader::UsesExtension(const std::string& ext)
{
	return !ext.compare("png");
}

void CPNGLoader::LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt)
{
	FILE* fh = fopen(path, "rb");
	if (fh == NULL)
		throw fileio_exception("Unable to open file for reading: " + std::string(path));
	LoadImage(fh, pimg, preffmt);
	fclose(fh);
}

void CPNGLoader::LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt)
{
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (png_ptr == NULL)
		throw fileio_exception("Unable to create read struct for PNG");

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		throw fileio_exception("Unable to create info struct for PNG");
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		throw fileio_exception("Unable to initialize PNG");
	}

	png_init_io(png_ptr, ifh);

	png_read_info(png_ptr, info_ptr);

	png_uint_32 width, height;
	int bit_depth, color_type, interlace_type;
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		&interlace_type, NULL, NULL);

	// future - upgrade support (for now, only RGBA 32-bit non-interlaced)
	if (bit_depth != 8 || color_type != PNG_COLOR_TYPE_RGB_ALPHA || interlace_type != PNG_INTERLACE_NONE)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		throw fileio_exception("Unsupported bit depth/color/interlace");
	}

	if (!pimg->Init(width, height, (preffmt == ImageFormat::None ? ImageFormat::RGBA : preffmt)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		throw fileio_exception("Unable to initialize PNG");
	}

	byte* pdata = new byte[png_get_rowbytes(png_ptr, info_ptr)];
	for (png_uint_32 y = 0; y < height; y++)
	{
		png_read_rows(png_ptr, &pdata, NULL, 1);
		pimg->WriteLine(y, pdata, ImageFormat::RGBA);
	}
	delete [] pdata;

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
}
