// image.h - defines an image buffer
//

#pragma once

#include <vector>
#include <memory>

#include "defines.h"
#include "fileio.h"
#include "rendertarget.h"

#include <string>
#include <map>

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CImageBuffer - an image buffer class
//-----------------------------------------------------------------------------

class CImageBuffer
{
protected:
	std::vector<byte> m_data;

	std::string m_path;
	int m_width;
	int m_height;
	ImageFormat m_fmt;
	int m_pxlsize;

protected:
	void Delete(void);

public:
	CImageBuffer(void);
	~CImageBuffer(void);

	bool Init(int w, int h, ImageFormat fmt);
	void SetPath(const char* path);
	bool WriteLine(int y, byte* pdata, ImageFormat fmt);

	bool GetSize(int& w, int& h) const;
	ImageFormat GetFormat(void) const;
	const char* GetPath(void) const;
	bool GetPixel(int x, int y, ImageFormat fmt, byte* pdata) const;
	bool GetLine(int y, ImageFormat fmt, byte* pdata) const;
	const std::vector<byte>& GetImageData(void) const;

	bool FillAlphaFromTransparentColor(byte r, byte g, byte b, byte dr, byte dg, byte db, byte a);
	bool FillAlphaFromImage(const CImageBuffer& img);
	bool FillAlphaFromColors(void);
	void NormalizeAlpha(void);

	void RenderStart(void);
	bool GetTexel(UVC uv, TextureFilter filter, COLOR& col) const;
	void RenderFinish(void);
};

//-----------------------------------------------------------------------------
// CImageBufferTarget - an image buffer class that can act as a rendering target
//-----------------------------------------------------------------------------

class CImageBufferTarget
	: public CImageBuffer, public CRenderTarget
{

public:
	CImageBufferTarget();
	~CImageBufferTarget();

	virtual REND_INFO GetRenderInfo(void) const;

	virtual void PreRender(int nthreads);
	virtual bool DoLine(int y, const dword *pline);
	virtual void PostRender(bool success);
};

//-----------------------------------------------------------------------------
// CImageLoader - abstract class for loading images
//-----------------------------------------------------------------------------

class CImageLoader
{
public:
	virtual bool UsesExtension(const std::string& ext) = 0;

	virtual void LoadImage(const char* path, CImageBuffer* pimg, ImageFormat preffmt) = 0;
	virtual void LoadImage(FILE* ifh, CImageBuffer* pimg, ImageFormat preffmt) = 0;
};

//-----------------------------------------------------------------------------
// CImageMap - a string keyed list of image buffers
//-----------------------------------------------------------------------------

typedef std::map<std::string, std::unique_ptr<CImageBuffer>> BufferMap;

class CImageMap
{
protected:
	BufferMap m_map;

	std::vector<std::unique_ptr<CImageLoader>> m_loaders;

public:
	CImageMap(void);
	~CImageMap(void);

	void AddLoader(std::unique_ptr<CImageLoader> loaderPtr);

	std::string LoadImageFile(const std::string& path, ImageFormat preffmt = ImageFormat::None, const std::string& title = "");
	std::string LoadImageFile(FILE* ifh, CImageLoader& loader, std::string title, ImageFormat preffmt = ImageFormat::None);
	void DeleteAll(void);

	CImageBuffer* GetImage(const std::string& title);
	const CImageBuffer* GetImage(const std::string& title) const;
	const BufferMap& GetBufferMap(void) const { return m_map; }

	// file I/O
	void Load(CFileBase& fobj);
	void Save(CFileBase& fobj);
};

_MIGRENDER_END
