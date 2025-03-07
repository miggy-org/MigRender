// jpegio.cpp - defines the JPEG I/O classes
//

#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include "../lib/jpeglib/jpeglib.h"
}

#include "jpegio.h"
#include "migexcept.h"
using namespace MigRender;

//-----------------------------------------------------------------------------
// CJPEGTarget
//-----------------------------------------------------------------------------

CJPEGTarget::CJPEGTarget()
	: CRenderTarget(), m_width(0), m_height(0), m_quality(0)
{
	m_cinfo = new jpeg_compress_struct;
	m_jerr = new jpeg_error_mgr;
	m_file = NULL;
}

CJPEGTarget::~CJPEGTarget()
{
	if (m_cinfo)
		delete m_cinfo;
	m_cinfo = NULL;
	if (m_jerr)
		delete m_jerr;
	m_jerr = NULL;
}

void CJPEGTarget::Init(int width, int height, int quality, const char *path)
{
	m_width = width;
	m_height = height;
	m_quality = quality;
	m_path = path;
}

REND_INFO CJPEGTarget::GetRenderInfo(void) const
{
	REND_INFO ri;
	ri.width = m_width;
	ri.height = m_height;
	ri.topdown = true;
	ri.fmt = ImageFormat::RGBA;
	return ri;
}

void CJPEGTarget::PreRender(int nthreads)
{
	if (nthreads > 1)
		throw prerender_exception("Image rendering does not support multi-threading yet");
	if (m_width == 0 || m_height == 0)
		throw prerender_exception("Width and height must be non-zero");

	m_cinfo->err = jpeg_std_error(m_jerr);
	jpeg_create_compress(m_cinfo);

	m_file = fopen(m_path.c_str(), "wb");
	if (m_file == NULL)
		throw fileio_exception("Unable to open file for writing: " + m_path);
	jpeg_stdio_dest(m_cinfo, m_file);

	m_cinfo->image_width = m_width;
	m_cinfo->image_height = m_height;
	m_cinfo->input_components = 3;
	m_cinfo->in_color_space = JCS_RGB;
	jpeg_set_defaults(m_cinfo);
	jpeg_set_quality(m_cinfo, m_quality, TRUE);

	jpeg_start_compress(m_cinfo, TRUE);
}

bool CJPEGTarget::DoLine(int y, const dword *pline)
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

	JSAMPROW row_pointer[1];
	row_pointer[0] = data.data();
	jpeg_write_scanlines(m_cinfo, row_pointer, 1);

	return true;
}

void CJPEGTarget::PostRender(bool success)
{
	jpeg_finish_compress(m_cinfo);
	if (m_file != NULL)
		fclose(m_file);
	jpeg_destroy_compress(m_cinfo);
	if (!success)
		remove(m_path.c_str());
}

//-----------------------------------------------------------------------------
// CJPEGLoader
//-----------------------------------------------------------------------------

bool CJPEGLoader::UsesExtension(const std::string& ext)
{
	return (!ext.compare("jpeg") || !ext.compare("jpg"));
}

void CJPEGLoader::LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt)
{
	if (pimg == NULL)
		throw fileio_exception("Image buffer is null: " + std::string(path));

	FILE* fh = fopen(path, "rb");
	if (fh == NULL)
		throw fileio_exception("Unable to open file for reading: " + std::string(path));

	LoadImage(fh, pimg, preffmt);
	fclose(fh);
}

void CJPEGLoader::LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt)
{
	if (pimg == NULL)
		throw fileio_exception("Image buffer is null");
	if (ifh == NULL)
		throw fileio_exception("FILE handle is null");

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, ifh);
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);

	if (!pimg->Init(cinfo.output_width, cinfo.output_height, (preffmt == ImageFormat::None ? ImageFormat::RGB : preffmt)))
		throw fileio_exception("Could not initialize image buffer");

	int row_stride = cinfo.output_width*cinfo.output_components;
	std::vector<byte> buffer(row_stride, 0);

	JSAMPROW row_pointer[1];
	row_pointer[0] = buffer.data();
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, row_pointer, 1);
		pimg->WriteLine(cinfo.output_scanline - 1, buffer.data(), ImageFormat::RGB);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
}
