#include "migexcept.h"

#include "oglrender.h"
#include "oglprogram.h"

#include <algorithm>

using namespace std;
using namespace MigRender;

// copied from https://docs.gl/gl3/glFrustum
static void LoadPerspectiveFovRH(CMatrix& ptm, double ulen, double vlen, double nearZ, double farZ)
{
    double left = -ulen / 2;
    double right = ulen / 2;
    double bottom = -vlen / 2;
    double top = vlen / 2;

    ptm[0] = 2 * nearZ / (right - left);
    ptm[2] = (right + left) / (right - left);
    ptm[5] = 2 * nearZ / (top - bottom);
    ptm[6] = (top + bottom) / (top - bottom);
    ptm[10] = -(farZ + nearZ) / (farZ - nearZ);
    ptm[11] = -(2 * farZ * nearZ) / (farZ - nearZ);
    ptm[14] = -1;
    ptm[15] = 0;
}

static double LoadDefaultPerspectiveMatrix(CMatrix& ptm, const CCamera& camera)
{
    double ulen, vlen, dist;
    camera.GetViewport(ulen, vlen, dist);

    double scale = 5;
    double nearZ = dist / scale;
    double farZ = scale * dist;
    LoadPerspectiveFovRH(ptm, ulen, vlen, nearZ, farZ);

    return scale;
}

// singleton instance
OglRender* OglRender::_theRender = nullptr;

OglRender* OglRender::GetInstance(void)
{
    if (_theRender == nullptr)
        throw mig_exception("OglRender singleton is NULL");
    return _theRender;
}

OglProgramBase* OglRender::GetProgram(OglProgramType programType)
{
    return GetInstance()->_thePrograms[programType].get();
}

OglMainProgram& OglRender::GetMainProgram(void)
{
    return *(GetInstance()->_mainProgram);
}

GLuint OglRender::GetTexture(const std::string& name)
{
    const auto& iter = GetInstance()->_mapTextures.find(name);
    if (iter == GetInstance()->_mapTextures.end())
        return 0;
    return iter->second;
}

void OglRender::AddTexture(const std::string& name, GLuint textureId)
{
    GetInstance()->_mapTextures[name] = textureId;
}

OglRender::OglRender(void) : _mainProgram(nullptr)
{
}

OglRender::~OglRender(void)
{
}

void OglRender::Init(void)
{
    if (_theRender != nullptr)
        throw mig_exception("Cannot have more than one active OglRender");
    _theRender = this;

    GLenum err = glewInit();
    if (err != GLEW_OK)
        throw mig_exception("glewInit() failed");

    // create all of the programs required (client will have to load them w/ shaders)
    _thePrograms[OglProgramType::Main] = std::make_unique<OglMainProgram>();
    _thePrograms[OglProgramType::Bg] = std::make_unique<OglProgramBase>();
    _mainProgram = static_cast<OglMainProgram*>(_thePrograms[OglProgramType::Main].get());

    _theBackground.Init();
}

void OglRender::Term(void)
{
    _theBackground.Clear();

    ResetModel();
    _mapTextures.clear();

    for (auto& iter : _thePrograms)
        iter.second->DeleteProgram();
    _mainProgram = nullptr;

    _theRender = nullptr;
}

void OglRender::LoadViewPerspectiveMatrices(const CCamera& camera)
{
    double scale = LoadDefaultPerspectiveMatrix(_tmPerspective, camera);
    _tmPerspective.Scale(scale, scale, 1);

    _tmView = camera.GetTM();
    _tmView.RotateY(180);

    _viewPos = CPt(0, 0, 0);
    camera.GetTM().TransformPoint(_viewPos, 1, MatrixType::CTM);
}

void OglRender::ProcessPolygon(const CModel& model, const CPolygon& poly, const CMatrix& itm)
{
    if (_mapObjects.find(&poly) != _mapObjects.end())
        _mapObjects[&poly]->Clear();

    auto newPoly = std::make_unique<OglPolygon>();
    newPoly->LoadPolygon(model, poly, itm);
    _mapObjects[&poly] = std::move(newPoly);
}

void OglRender::ProcessSphere(const CModel& model, const CSphere& sphere, const CMatrix& itm)
{
    if (_mapObjects.find(&sphere) != _mapObjects.end())
        _mapObjects[&sphere]->Clear();

    auto newPoly = std::make_unique<OglSphere>();
    newPoly->LoadSphere(model, sphere, itm);
    _mapObjects[&sphere] = std::move(newPoly);
}

void OglRender::ProcessLight(const CModel& model, const CLight& lit, const CMatrix& itm)
{
}

void OglRender::ProcessGroup(const CModel& model, const CGrp& grp, const CMatrix& itm)
{
    for (const auto& lit : grp.GetLights())
    {
        CMatrix tm = lit->GetTM();
        tm.MatrixMultiply(itm);

        ProcessLight(model, *lit, tm);
    }

    for (const auto& subgrp : grp.GetSubGroups())
    {
        CMatrix tm = subgrp->GetTM();
        tm.MatrixMultiply(itm);

        ProcessGroup(model, *subgrp, tm);
    }

    for (const auto& obj : grp.GetObjects())
    {
        CMatrix tm = obj->GetTM();
        tm.MatrixMultiply(itm);

        if (obj->GetType() == ObjType::Polygon)
            ProcessPolygon(model, *static_cast<CPolygon*>(obj.get()), tm);
        else if (obj->GetType() == ObjType::Sphere)
            ProcessSphere(model, *static_cast<CSphere*>(obj.get()), tm);
    }
}

bool OglRender::FindAndUpdateObject(const CModel& model, const CGrp& grp, const CMatrix& itm, const CBaseObj* pobj)
{
    ObjType type = pobj->GetType();

    if (type == ObjType::DirLight || type == ObjType::PtLight || type == ObjType::SpotLight)
    {
        for (const auto& lit : grp.GetLights())
        {
            if (lit.get() == (CLight*)pobj)
            {
                CMatrix tm = lit->GetTM();
                tm.MatrixMultiply(itm);

                ProcessLight(model, *lit, tm);
                return true;
            }
        }
    }

    if (type == ObjType::Group)
    {
        for (const auto& subgrp : grp.GetSubGroups())
        {
            if (subgrp.get() == (CGrp*)pobj)
            {
                CMatrix tm = subgrp->GetTM();
                tm.MatrixMultiply(itm);

                ProcessGroup(model, *subgrp, tm);
                return true;
            }
        }
    }

    if (type == ObjType::Polygon)
    {
        for (const auto& obj : grp.GetObjects())
        {
            if (obj.get() == (CPolygon*)pobj)
            {
                CMatrix tm = obj->GetTM();
                tm.MatrixMultiply(itm);

                ProcessPolygon(model, *static_cast<CPolygon*>(obj.get()), tm);
                return true;
            }
        }
    }

    for (const auto& subgrp : grp.GetSubGroups())
    {
        CMatrix tm = subgrp->GetTM();
        tm.MatrixMultiply(itm);

        if (FindAndUpdateObject(model, *subgrp, tm, pobj))
            return true;
    }

    return false;
}

void OglRender::RenderLights(const CModel& model, const CGrp& grp, const CMatrix& itm, int& lightIndex)
{
    for (const auto& subgrp : grp.GetSubGroups())
    {
        CMatrix tm = subgrp->GetTM();
        tm.MatrixMultiply(itm);

        RenderLights(model, *subgrp, tm, lightIndex);
    }

    for (const auto& lit : grp.GetLights())
    {
        if (lightIndex < MAX_NUMBER_OF_LIGHTS)
        {
            GLint type = LIGHT_TYPE_OFF;
            CPt pt;

            if (lit->GetType() == ObjType::DirLight)
            {
                const CDirLight* dlit = static_cast<const CDirLight*>(lit.get());
                pt = dlit->GetDirection();
                type = LIGHT_TYPE_DIR;
            }
            else if (lit->GetType() == ObjType::PtLight)
            {
                const CPtLight* plit = static_cast<const CPtLight*>(lit.get());
                pt = plit->GetOrigin();
                type = LIGHT_TYPE_POINT;
            }

            if (type != LIGHT_TYPE_OFF)
            {
                CMatrix tm = lit->GetTM();
                tm.MatrixMultiply(itm);

                itm.TransformPoint(pt, (type == LIGHT_TYPE_DIR ? 0 : 1), MatrixType::CTM);

                GetMainProgram().LoadLight(lightIndex++, type, pt, lit->GetColor());
            }
        }
    }
}

void OglRender::RenderAllLights(const CModel& model)
{
    int nextLight = 0;
    RenderLights(model, model.GetSuperGroup(), model.GetSuperGroup().GetTM(), nextLight);
}

static bool IsTransparent(const CObj& obj)
{
    // MigRender also has a refraction map, but we won't support that here
    return (obj.GetRefraction().a < 1 || !obj.GetTextures(TextureMapType::Transparency).empty());
}

void OglRender::RenderGroup(const CModel& model, const CGrp& grp, const CMatrix& itm)
{
    for (const auto& subgrp : grp.GetSubGroups())
    {
        CMatrix tm = subgrp->GetTM();
        tm.MatrixMultiply(itm);

        RenderGroup(model, *subgrp, tm);
    }

    for (const auto& obj : grp.GetObjects())
    {
        CMatrix tm = obj->GetTM();
        tm.MatrixMultiply(itm);

        if (obj->GetType() == ObjType::Polygon || obj->GetType() == ObjType::Sphere)
        {
            const auto& iter = _mapObjects.find(&(*obj));
            if (iter != _mapObjects.end())
            {
                if (!IsTransparent(*obj))
                {
                    // object is non-transparent, render now
                    iter->second->Render(model, *(obj.get()), tm);
                }
                else
                {
                    // get center of object in local coords
                    CPt center;
                    if (_mapCenterPoints.find(obj.get()) == _mapCenterPoints.end())
                    {
                        // note that this cache assumes the object never changes it's bounding box in local coords
                        CBoundBox bbox;
                        obj->ComputeBoundBox(bbox, CMatrix());
                        center = bbox.GetCenter();
                        _mapCenterPoints[obj.get()] = center;
                    }
                    else
                        center = _mapCenterPoints[obj.get()];

                    // transform to world coords
                    tm.TransformPoint(center, 1, MatrixType::CTM);

                    // save for later
                    OglDeferredRender dr;
                    dr.distance = CPt(center - _viewPos).GetLength();
                    dr.obj = obj.get();
                    dr.oglObject = iter->second.get();
                    dr.tm = tm;
                    _deferredRenders.push_back(dr);
                }
            }
        }
    }
}

void OglRender::RenderSuperGroup(const CModel& model)
{
    RenderGroup(model, model.GetSuperGroup(), model.GetSuperGroup().GetTM());
}

void OglRender::RenderDeferredObjects(const CModel& model)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // sort by distance, farthest first
    std::sort(_deferredRenders.begin(), _deferredRenders.end(),
        [](const OglDeferredRender& a, const OglDeferredRender& b) { return a.distance > b.distance; });

    for (const auto& iter : _deferredRenders)
        iter.oglObject->Render(model, *(iter.obj), iter.tm);

    _deferredRenders.clear();
    glDisable(GL_BLEND);
}

void OglRender::BuildModel(const CModel& model)
{
    CMatrix tmSuper = model.GetSuperGroup().GetTM();
    ProcessGroup(model, model.GetSuperGroup(), tmSuper);

    _theBackground.Load(model);
}

bool OglRender::UpdateObject(const CModel& model, const CBaseObj* pobj)
{
    if (pobj == NULL)
        throw mig_exception("Cannot update a NULL object");

    return FindAndUpdateObject(model, model.GetSuperGroup(), model.GetSuperGroup().GetTM(), pobj);
}

void OglRender::ResetModel(bool resetTextures)
{
    _mapObjects.clear();
    _mapCenterPoints.clear();
    if (resetTextures)
        _mapTextures.clear();
}

void OglRender::Render(const CModel& model)
{
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // prepare view and perspective matrices
    LoadViewPerspectiveMatrices(model.GetCamera());

    // render background
    _theBackground.Render(model);

    // activate the main shader program
    GetMainProgram().UseProgram();

    // model globals (ambient, fog, fade, etc)
    GetMainProgram().LoadModelGlobals(model);

    // lights
    RenderAllLights(model);

    // main group which contains the entire model
    RenderSuperGroup(model);

    // render transparent objects that were skipped in the previous step
    RenderDeferredObjects(model);
}
