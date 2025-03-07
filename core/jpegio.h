// jpegio.h - defines the JPEG I/O classes
//

#pragma once

#include "rendertarget.h"
#include "image.h"

struct jpeg_compress_struct;
struct jpeg_error_mgr;

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CJPEGTarget - a rendering target class that writes a JPEG file
//-----------------------------------------------------------------------------

class CJPEGTarget
	: public CRenderTarget
{
protected:
	std::string m_path;
	int m_width;
	int m_height;
	int m_quality;

private:
	struct jpeg_compress_struct *m_cinfo;
	struct jpeg_error_mgr *m_jerr;
	FILE *m_file;

public:
	CJPEGTarget();
	~CJPEGTarget();

	void Init(int width, int height, int quality, const char *path);

	virtual REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CJPEGLoader - a class for loading JPEG images
//-----------------------------------------------------------------------------

class CJPEGLoader : public CImageLoader
{
public:
	bool UsesExtension(const std::string& ext);

	void LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt);
	void LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt);
};

_MIGRENDER_END
