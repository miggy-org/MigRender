// bmpio.h - defines the BMP I/O classes
//

#pragma once

#include "rendertarget.h"
#include "image.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CBMPTarget - a rendering target class that writes a BMP file
//-----------------------------------------------------------------------------

class CBMPTarget
	: public CRenderTarget
{
protected:
	std::string m_path;
	int m_width;
	int m_height;
	int m_bitcnt;

private:
	FILE *m_file;

public:
	CBMPTarget();
	~CBMPTarget();

	void Init(int width, int height, int bitcnt, const char *path);

	virtual REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CBMPLoader - a class for loading BMP images
//-----------------------------------------------------------------------------

class CBMPLoader : public CImageLoader
{
public:
	bool UsesExtension(const std::string& ext);

	void LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt);
	void LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt);
};

_MIGRENDER_END
