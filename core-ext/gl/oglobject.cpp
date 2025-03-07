#include "oglrender.h"

using namespace MigRender;

static GLuint LoadTextureMap(const CModel& model, const CTexture* pptxt)
{
    const std::string& mapName = pptxt->GetMapName();
    GLuint textureId = OglRender::GetTexture(mapName);
    if (textureId == 0)
    {
        textureId = LoadTextureMap(model.GetImageMap().GetImage(mapName));
        if (textureId > 0)
            OglRender::AddTexture(mapName, textureId);
    }

    return textureId;
}

static std::vector<GLfloat> _tessVerts;
static std::vector<GLushort> _tessIndex;

static void PushBackVertexData(const CPt& pt)
{
    _tessVerts.push_back(static_cast<GLfloat>(pt.x));
    _tessVerts.push_back(static_cast<GLfloat>(pt.y));
    _tessVerts.push_back(static_cast<GLfloat>(pt.z));
}

static void PushBackVertexData(const UVC& uv)
{
    _tessVerts.push_back(static_cast<GLfloat>(uv.u));
    _tessVerts.push_back(static_cast<GLfloat>(uv.v));
}

///////////////////////////////////////////////////////////////////////////////
// OglObject base implementation
//

OglObject::OglObject(void) : _vertexArray(-1)
{
}

static GLint TextureOpToGLTextureOp(TextureMapOp op)
{
    if (op == TextureMapOp::Add)
        return TEX_OP_ADD;
    else if (op == TextureMapOp::Subtract)
        return TEX_OP_SUBTRACT;
    else if (op == TextureMapOp::Blend)
        return TEX_OP_BLEND;
    return TEX_OP_MULTIPLY;
}

static GLint TextureFlagsToGLTextureFlags(dword flags)
{
    GLint glFlags = 0;
    if (flags & TXTF_INVERT)
        glFlags |= TEX_FLAG_INVERT_COLOR;
    if (flags & TXTF_INVERT_ALPHA)
        glFlags |= TEX_FLAG_INVERT_ALPHA;
    return glFlags;
}

void OglObject::LoadTextureMaps(const CModel& model, const TEXTURES& maps, GLint type, std::vector<const CTexture*>& listTextures)
{
    for (const auto& txt : maps)
    {
        const CTexture* ptxt = static_cast<const CTexture*>(txt.get());
        if (ptxt->IsEnabled())
        {
            GLuint textureId = LoadTextureMap(model, ptxt);

            GLint index = -1;
            for (GLuint i = 0; i < _textureIds.size() && index == -1; i++)
            {
                if (_textureIds[i] == textureId)
                    index = i;
            }
            if (index == -1)
            {
                _textureIds.push_back(textureId);
                index = _textureIds.size() - 1;
            }

            OglTextureBlock newBlock;
            newBlock.type = type;
            newBlock.coordIndex = (type != TEX_TYPE_REFLECTION ? listTextures.size() : -1);
            newBlock.samplerIndex = index;
            newBlock.op = TextureOpToGLTextureOp(ptxt->GetOperation());
            newBlock.flags = TextureFlagsToGLTextureFlags(ptxt->GetFlags());
            _textureBlocks.push_back(newBlock);

            if (newBlock.coordIndex != -1)
                listTextures.push_back(ptxt);
        }
    }
}

void OglObject::LoadTextureMaps(const CModel& model, const CObj& poly, std::vector<const CTexture*>& listTextures)
{
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Diffuse), TEX_TYPE_DIFFUSE, listTextures);
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Specular), TEX_TYPE_SPECULAR, listTextures);
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Transparency), TEX_TYPE_TRANSPARENCY, listTextures);
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Glow), TEX_TYPE_GLOW, listTextures);
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Reflection), TEX_TYPE_REFLECTION, listTextures);
    LoadTextureMaps(model, poly.GetTextures(TextureMapType::Bump), TEX_TYPE_BUMP, listTextures);
}

void OglObject::LoadTesselatedData(GLsizei numTexturesInVertexData)
{
    // compute stride (amount of vertex data per actual vertex)
    GLsizei stride = 6 + 2 * numTexturesInVertexData;

    // this will hold our entire object structure in memory
    glGenVertexArrays(1, &_vertexArray);
    glBindVertexArray(_vertexArray);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, _tessVerts.size() * sizeof(GLfloat), _tessVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(OglRender::GetMainProgram().GetVertexPosition(), 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(OglRender::GetMainProgram().GetVertexPosition());
    glVertexAttribPointer(OglRender::GetMainProgram().GetVertexNormalPosition(), 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(OglRender::GetMainProgram().GetVertexNormalPosition());
    for (size_t i = 0; i < _textureBlocks.size(); i++)
    {
        glVertexAttribPointer(OglRender::GetMainProgram().GetVertexTexPosition(i), 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void*)((6 + 2 * i) * sizeof(GLfloat)));
        glEnableVertexAttribArray(OglRender::GetMainProgram().GetVertexTexPosition(i));
    }

    // bind the element array next
    GLuint elementArrayBuffer;
    glGenBuffers(1, &elementArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, _tessIndex.size() * sizeof(GLushort), _tessIndex.data(), GL_STATIC_DRAW);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // not needed anymore
    _tessVerts.clear();
    _tessIndex.clear();
}

void OglObject::Clear(void)
{
    if (_vertexArray != -1)
        glDeleteVertexArrays(1, &_vertexArray);
    _vertexArray = -1;

    _textureIds.clear();
    _textureBlocks.clear();
    _curves.clear();
}

void OglObject::Render(const CModel& model, const CObj& obj, const CMatrix& itm) const
{
    //glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_TRUE);

    OglRender::GetMainProgram().LoadMatrices(itm);

    OglRender::GetMainProgram().LoadObject(obj);

    OglRender::GetMainProgram().LoadTextureSamplers(_textureIds);

    OglRender::GetMainProgram().LoadTextureBlocks(_textureBlocks);

    glBindVertexArray(_vertexArray);

    for (const OglCurves& curves : _curves)
    {
        CPt norm(curves.norm.x, curves.norm.y, curves.norm.z);
        itm.TransformPoint(norm, 0, MatrixType::CTM);
        OglRender::GetMainProgram().LoadNormal(norm);

        for (const OglCurve& curve : curves.curves)
        {
            glDrawElements(curve.type, curve.cnt, GL_UNSIGNED_SHORT, reinterpret_cast<const void*>(curve.offset * sizeof(GLushort)));
            CheckGLError("Draw elements error");
        }
    }

    glBindVertexArray(0);
}

///////////////////////////////////////////////////////////////////////////////
// OglPolygon implementation
//

static OglCurve _nextCurve;

static void tessBegin(GLenum type, void* data)
{
    //OutputDebugStringA("TESS: begin\n");
    _nextCurve.type = type;
    _nextCurve.offset = _tessIndex.size();
    _nextCurve.cnt = 0;
}

static void tessEdgeFlag(GLboolean flag, void* data)
{
    //OutputDebugStringA("TESS: edgeFlag\n");
}

static void tessVertex(void* vertexData, void* data)
{
    //OutputDebugStringA("TESS: vertex\n");
    _tessIndex.push_back((int)vertexData);
    _nextCurve.cnt++;
}

static void tessEnd(void* data)
{
    //OutputDebugStringA("TESS: end\n");
    if (data != NULL)
        (*((std::vector<OglCurves>*)data)).back().curves.push_back(_nextCurve);
}

static void tessCombine(GLdouble coords[3], void* vertex_data[4], GLfloat weight[4], void** outData, void* data)
{
    //OutputDebugStringA("TESS: combine\n");
}

//#define GLU_TESS_MISSING_BEGIN_POLYGON  100151
//#define GLU_TESS_MISSING_BEGIN_CONTOUR  100152
//#define GLU_TESS_MISSING_END_POLYGON    100153
//#define GLU_TESS_MISSING_END_CONTOUR    100154
//#define GLU_TESS_COORD_TOO_LARGE        100155
//#define GLU_TESS_NEED_COMBINE_CALLBACK  100156
static void tessError(GLenum error, void* data)
{
    //OutputDebugStringA("TESS: error\n");
}

static void TessellatePolygon(const CPolygon& poly, const std::vector<const CTexture*>& maps, std::vector<OglCurves>& curves)
{
    GLUtesselator* tess = gluNewTess();

    gluTessCallback(tess, GLU_TESS_BEGIN_DATA, (void(__stdcall*)()) tessBegin);
    //gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, (void(__stdcall*)()) tessEdgeFlag);
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (void(__stdcall*)()) tessVertex);
    gluTessCallback(tess, GLU_TESS_END_DATA, (void(__stdcall*)()) tessEnd);
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA, (void(__stdcall*)()) tessCombine);
    gluTessCallback(tess, GLU_TESS_ERROR_DATA, (void(__stdcall*)()) tessError);

    // stride is 3 (vertex) + 3 (norm) + 2 times number of textures
    GLsizei stride = 6 + 2 * maps.size();

    GLdouble coords[3];
    for (int i = 0; i < poly.GetCurves().Count(); i++)
    {
        const POLY_CURVE& curve = poly.GetCurves()[i];

        if (curve.flags & PCF_NEWPLANE)
        {
            if (i > 0)
                gluTessEndPolygon(tess);

            curves.push_back(OglCurves());
            curves.back().norm = curve.norm;
            gluTessBeginPolygon(tess, (void*)&curves);
        }

        std::map<GLushort, GLushort> mapOrigIndexToNewIndex;

        gluTessBeginContour(tess);
        for (int j = 0; j < curve.cnt; j++)
        {
            int index = poly.GetIndices()[curve.sind + j];
            const CPt& vert = poly.GetLattice()[index];
            coords[0] = vert.x;
            coords[1] = vert.y;
            coords[2] = vert.z;

            int realIndex = index;
            if (mapOrigIndexToNewIndex.find(index) == mapOrigIndexToNewIndex.end())
            {
                PushBackVertexData(vert);
                if (curve.flags & PCF_VERTEXNORMS)
                    PushBackVertexData(poly.GetNormals()[curve.snorm + j]);
                else
                    PushBackVertexData(curve.norm);

                for (const CTexture* pptxt : maps)
                {
                    const CPolyTexture* ppolytxt = static_cast<const CPolyTexture*>(pptxt);
                    if (ppolytxt != NULL)
                    {
                        bool isind = ppolytxt->IsInd();
                        int mappingIndex = (isind ? curve.smap : curve.sind);
                        if (mappingIndex > -1)
                        {
                            mappingIndex += j;
                            UVC uv = ppolytxt->GetConstrainedUVC(ppolytxt->GetCoord(isind ? mappingIndex : poly.GetIndices()[mappingIndex]));
                            PushBackVertexData(UVC(uv.u, 1 - uv.v));
                        }
                        else
                            PushBackVertexData(UVC(0, 0));
                    }
                }
                realIndex = (_tessVerts.size() / stride) - 1;
            }
            else
                realIndex = mapOrigIndexToNewIndex[index];
            gluTessVertex(tess, coords, (void*)(realIndex));
        }
        mapOrigIndexToNewIndex.clear();

        gluTessEndContour(tess);
    }
    gluEndPolygon(tess);

    gluDeleteTess(tess);
}

OglPolygon::OglPolygon(void) : OglObject()
{
}

void OglPolygon::LoadPolygon(const CModel& model, const CPolygon& poly, const CMatrix& itm)
{
    // load texture maps, and generate a complete list of textures being used
    std::vector<const CTexture*> listTextures;
    LoadTextureMaps(model, poly, listTextures);

    // tesselate (builds up _tessVerts and _tessIndex)
    TessellatePolygon(poly, listTextures, _curves);

    // tesselated data is ready, load it
    LoadTesselatedData(listTextures.size());
}

void OglPolygon::Render(const CModel& model, const CObj& obj, const CMatrix& itm) const
{
    glDisable(GL_CULL_FACE);

    OglObject::Render(model, obj, itm);
}

///////////////////////////////////////////////////////////////////////////////
// OglSphere implementation
//

static UVC ComputeSphericalTexelCoord(const CTexture* pptxt, CUnitVector norm)
{
    UVC ret;
    double origY = norm.y;

    norm.x = -norm.x;
    norm.y = 0;
    if (norm.Normalize() > 0)
    {
        ret.u = atan2(norm.z, norm.x);
        if (ret.u >= 0)
            ret.u = ret.u / (2 * M_PI);
        else
            ret.u = (M_PI + ret.u) / (2 * M_PI) + 0.5;
    }
    else
        ret.u = 0.5;
    ret.v = (1 - origY) / 2.0;

    return pptxt->GetConstrainedUVC(ret);
}

static void GenerateSphere(const CSphere& sphere, const std::vector<const CTexture*>& maps, std::vector<OglCurves>& curves)
{
    // one division every 5 degrees
    int ydiv = 36;
    int xzdiv = 72;

    // stride is 3 (vertex) + 3 (norm) + 2 times number of textures
    GLsizei stride = 6 + 2 * maps.size();

    CPt origin = sphere.GetOrigin();
    double radius = sphere.GetRadius();

    // vertex strips
    for (int y = 1; y < ydiv; y++)
    {
        CPt pt;
        pt.y = -cos(ConvertToRadians((180.0 / ydiv) * y)) * radius + origin.x;

        double scaleXZ = sin(ConvertToRadians((180.0 / ydiv) * y));
        for (int xz = 0; xz <= xzdiv; xz++)
        {
            double angle;
            if (xz == 0)
                angle = -90;    // ensures U tex coord will be 0
            else if (xz == xzdiv)
                angle = -90.01; // ensures U tex coord will be 1
            else
                angle = (360.0 / xzdiv) * xz - 90;
            pt.x = scaleXZ * sin(ConvertToRadians(angle)) * radius + origin.x;
            pt.z = scaleXZ * cos(ConvertToRadians(angle)) * radius + origin.x;

            PushBackVertexData(pt);

            CUnitVector norm = pt - origin;
            norm.Normalize();
            PushBackVertexData(norm);

            for (const CTexture* pptxt : maps)
                PushBackVertexData(ComputeSphericalTexelCoord(pptxt, norm));
        }
    }

    // cap vertices
    PushBackVertexData(CPt(origin.x, origin.y - radius, origin.z));
    PushBackVertexData(CPt(0, -1, 0));
    for (const CTexture* pptxt : maps)
        PushBackVertexData(ComputeSphericalTexelCoord(pptxt, CUnitVector(0, -1, 0)));
    PushBackVertexData(CPt(origin.x, origin.y + radius, origin.z));
    PushBackVertexData(CPt(0, 1, 0));
    for (const CTexture* pptxt : maps)
        PushBackVertexData(ComputeSphericalTexelCoord(pptxt, CUnitVector(0, 1, 0)));

    // triangle strip index buffers
    for (int y = 0; y < ydiv - 2; y++)
    {
        OglCurve curve;
        curve.type = GL_TRIANGLE_STRIP;
        curve.offset = _tessIndex.size();
        curve.cnt = 2 * xzdiv + 2;
        curves.push_back(OglCurves());
        curves.back().curves.push_back(curve);

        int baseBottomStrip = (xzdiv + 1) * y;
        int baseTopStrip = baseBottomStrip + (xzdiv + 1);
        for (int xz = 0; xz <= xzdiv; xz++)
        {
            _tessIndex.push_back(baseTopStrip + xz);
            _tessIndex.push_back(baseBottomStrip + xz);
        }
        //_tessIndex.push_back(baseBottomStrip);
        //_tessIndex.push_back(baseTopStrip);
    }

    // triangle fan caps index buffers
    OglCurve curve;
    curve.type = GL_TRIANGLE_FAN;
    curve.offset = _tessIndex.size();
    curve.cnt = xzdiv + 1;
    curves.push_back(OglCurves());
    curves.back().curves.push_back(curve);
    _tessIndex.push_back((xzdiv + 1) * (ydiv - 1));
    for (int xz = xzdiv; xz >= 0; xz--)
        _tessIndex.push_back(xz);
    //_tessIndex.push_back(0);

    curve.type = GL_TRIANGLE_FAN;
    curve.offset = _tessIndex.size();
    curve.cnt = xzdiv + 1;
    curves.push_back(OglCurves());
    curves.back().curves.push_back(curve);
    _tessIndex.push_back((xzdiv + 1) * (ydiv - 1) + 1);
    for (int xz = 0; xz <= xzdiv; xz++)
        _tessIndex.push_back((xzdiv + 1) * (ydiv - 2) + xz);
    //_tessIndex.push_back(ydiv * (ydiv - 1));
}

OglSphere::OglSphere(void) : OglObject()
{
}

void OglSphere::LoadSphere(const CModel& model, const CSphere& sphere, const CMatrix& itm)
{
    // load texture maps, and generate a complete list of textures being used
    std::vector<const CTexture*> listTextures;
    LoadTextureMaps(model, sphere, listTextures);

    // build the sphere geometry (builds up _tessVerts and _tessIndex)
    GenerateSphere(sphere, listTextures, _curves);

    // tesselated data is ready, load it
    LoadTesselatedData(listTextures.size());
}

void OglSphere::Render(const CModel& model, const CObj& obj, const CMatrix& itm) const
{
    glEnable(GL_CULL_FACE);

    OglObject::Render(model, obj, itm);
}
