// pngio.h - defines the PNG I/O classes
//

#pragma once

#include "rendertarget.h"
#include "image.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CPNGTarget - a rendering target class that writes a PNG file
//-----------------------------------------------------------------------------

class CPNGTarget
	: public CRenderTarget
{
protected:
	std::string m_path;
	int m_width;
	int m_height;

private:
	void* m_pngptr;
	void* m_info;
	FILE *m_file;

	void PNGDestroy(void);

public:
	CPNGTarget();
	~CPNGTarget();

	void Init(int width, int height, const char *path);

	virtual REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CPNGLoader - a class for loading PNG images
//-----------------------------------------------------------------------------

class CPNGLoader : public CImageLoader
{
public:
	bool UsesExtension(const std::string& ext);

	void LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt);
	void LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt);
};

_MIGRENDER_END
