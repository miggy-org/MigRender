#pragma once

#include "model.h"

#include "oglcommon.h"
#include "oglprogram.h"

_MIGRENDER_BEGIN

struct OglCurve
{
    GLenum type;
    GLsizei offset;
    GLsizei cnt;
};

struct OglCurves
{
    CPt norm;
    std::vector<OglCurve> curves;
};

class OglObject
{
protected:
    std::vector<OglTextureBlock> _textureBlocks;
    std::vector<GLuint> _textureIds;
    GLuint _vertexArray;

    std::vector<OglCurves> _curves;

protected:
    void LoadTextureMaps(const CModel& model, const TEXTURES& maps, GLint type, std::vector<const CTexture*>& listTextures);
    void LoadTextureMaps(const CModel& model, const CObj& poly, std::vector<const CTexture*>& listTextures);
    void LoadTesselatedData(GLsizei numTexturesInVertexData);

public:
	OglObject(void);

    virtual void Clear(void);

	virtual void Render(const CModel& model, const CObj& obj, const CMatrix& itm) const;
};

class OglPolygon : public OglObject
{
public:
    OglPolygon(void);

    void LoadPolygon(const CModel& model, const CPolygon& poly, const CMatrix& itm);

    virtual void Render(const CModel& model, const CObj& obj, const CMatrix& itm) const;
};

class OglSphere : public OglObject
{
public:
    OglSphere(void);

    void LoadSphere(const CModel& model, const CSphere& sphere, const CMatrix& itm);

    virtual void Render(const CModel& model, const CObj& obj, const CMatrix& itm) const;
};

_MIGRENDER_END
