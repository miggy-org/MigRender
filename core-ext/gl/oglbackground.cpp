#include "oglrender.h"

using namespace MigRender;

OglBackground::OglBackground(void) : _bgTextureId(0), _vertexArray(-1), _bgProgram(nullptr)
{
}

void OglBackground::Init(void)
{
    _bgProgram = OglRender::GetProgram(OglProgramType::Bg);
}

void OglBackground::Clear(void)
{
    if (_vertexArray != -1)
        glDeleteVertexArrays(1, &_vertexArray);
    _vertexArray = -1;
    _bgProgram = nullptr;
}

void OglBackground::Load(const CModel& model)
{
    double bgAspect = 0;

    std::string bgName;
    ImageResize resize;
    model.GetBackgroundHandler().GetBackgroundImage(bgName, resize);
    if (!bgName.empty())
    {
        _bgTextureId = OglRender::GetTexture(bgName);
        if (_bgTextureId == 0)
        {
            const CImageBuffer* pbuf = model.GetImageMap().GetImage(bgName);
            if (pbuf != NULL)
            {
                _bgTextureId = LoadTextureMap(pbuf);
                if (_bgTextureId > 0)
                    OglRender::AddTexture(bgName, _bgTextureId);

                int w, h;
                pbuf->GetSize(w, h);
                bgAspect = w / (double)h;
            }
        }
    }

    double ulen, vlen, dist;
    model.GetCamera().GetViewport(ulen, vlen, dist);
    double vpAspect = ulen / vlen;

    UVC uvMin(0, 0), uvMax(1, 1);
    if (bgAspect > 0)
    {
        if (resize == ImageResize::ScaleToFit)
        {
            if (vpAspect > bgAspect)
            {
                double hr = vpAspect / bgAspect;
                uvMin.u = -(hr - 1) / 2.0;
                uvMax.u = hr - (hr - 1) / 2.0;
            }
            else
            {
                double hr = bgAspect / vpAspect;
                uvMin.v = -(hr - 1) / 2.0;
                uvMax.v = hr - (hr - 1) / 2.0;
            }
        }
        else if (resize == ImageResize::ScaleToFill)
        {
            if (vpAspect > bgAspect)
            {
                double hr = bgAspect / vpAspect;
                uvMin.v = (1 - hr) / 2.0;
                uvMax.v = (1 - hr) / 2.0 + hr;
            }
            else
            {
                double hr = vpAspect / bgAspect;
                uvMin.u = (1 - hr) / 2.0;
                uvMax.u = (1 - hr) / 2.0 + hr;
            }
        }
    }

    GLsizei stride = 5;
    std::vector<GLfloat> verts;
    std::vector<GLushort> indecies;
    verts.push_back(static_cast<GLfloat>(-ulen / 2));
    verts.push_back(static_cast<GLfloat>(-vlen / 2));
    verts.push_back(static_cast<GLfloat>(0));
    verts.push_back(static_cast<GLfloat>(uvMin.u));
    verts.push_back(static_cast<GLfloat>(uvMax.v));
    verts.push_back(static_cast<GLfloat>( ulen / 2));
    verts.push_back(static_cast<GLfloat>(-vlen / 2));
    verts.push_back(static_cast<GLfloat>(0));
    verts.push_back(static_cast<GLfloat>(uvMax.u));
    verts.push_back(static_cast<GLfloat>(uvMax.v));
    verts.push_back(static_cast<GLfloat>( ulen / 2));
    verts.push_back(static_cast<GLfloat>( vlen / 2));
    verts.push_back(static_cast<GLfloat>(0));
    verts.push_back(static_cast<GLfloat>(uvMax.u));
    verts.push_back(static_cast<GLfloat>(uvMin.v));
    verts.push_back(static_cast<GLfloat>(-ulen / 2));
    verts.push_back(static_cast<GLfloat>( vlen / 2));
    verts.push_back(static_cast<GLfloat>(0));
    verts.push_back(static_cast<GLfloat>(uvMin.u));
    verts.push_back(static_cast<GLfloat>(uvMin.v));

    indecies.push_back(static_cast<GLushort>(0));
    indecies.push_back(static_cast<GLushort>(1));
    indecies.push_back(static_cast<GLushort>(2));
    indecies.push_back(static_cast<GLushort>(3));

    // this will hold our entire object structure in memory
    glGenVertexArrays(1, &_vertexArray);
    glBindVertexArray(_vertexArray);

    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    GLuint vertexBuffer;
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(GLfloat), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(_bgProgram->GetAttributePosition("vPosition"), 3, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(_bgProgram->GetAttributePosition("vPosition"));
    glVertexAttribPointer(_bgProgram->GetAttributePosition("vTex"), 2, GL_FLOAT, GL_FALSE, stride * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(_bgProgram->GetAttributePosition("vTex"));

    // bind the element array next
    GLuint elementArrayBuffer;
    glGenBuffers(1, &elementArrayBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementArrayBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indecies.size() * sizeof(GLushort), indecies.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void OglBackground::Render(const CModel& model)
{
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);

    COLOR colNorth, colEquator, colSouth;
    model.GetBackgroundHandler().GetBackgroundColors(colNorth, colEquator, colSouth);

    const CMatrix& matCamera = OglRender::GetInstance()->GetViewMatrix();
    CPt viewPos;
    matCamera.TransformPoint(viewPos, 1, MatrixType::CTM);

    CMatrix mvp;
    mvp.MatrixMultiply(matCamera);
    mvp.MatrixMultiply(OglRender::GetInstance()->GetPerspectiveMatrix());

    _bgProgram->UseProgram();
    _bgProgram->SetUniformMatrix("matMVP", false, mvp);
    _bgProgram->SetUniform4f("colNorth", colNorth.r, colNorth.g, colNorth.b, colNorth.a);
    _bgProgram->SetUniform4f("colEquator", colEquator.r, colEquator.g, colEquator.b, colEquator.a);
    _bgProgram->SetUniform4f("colSouth", colSouth.r, colSouth.g, colSouth.b, colSouth.a);
    _bgProgram->SetUniform4f("colFog", model.GetFog().r, model.GetFog().g, model.GetFog().b, model.GetFog().a);
    _bgProgram->SetUniform4f("colFade", model.GetFade().r, model.GetFade().g, model.GetFade().b, model.GetFade().a);
    _bgProgram->SetUniform3f("viewPos", viewPos.x, viewPos.x, viewPos.z);
    _bgProgram->SetUniform1i("useFog", (model.GetFogNear() > 0 && model.GetFogFar() > 0) ? 1 : 0);
    _bgProgram->SetUniform1i("useTexture", (_bgTextureId > 0 ? 1 : 0));

    if (_bgTextureId > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _bgTextureId);
        _bgProgram->SetUniform1i("bgTexture", 0);
    }

    glBindVertexArray(_vertexArray);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_SHORT, reinterpret_cast<const void*>(0));
    glBindVertexArray(0);
}
