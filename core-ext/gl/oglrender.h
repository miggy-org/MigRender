#pragma once

#include "model.h"

#include "oglcommon.h"
#include "oglobject.h"
#include "oglprogram.h"
#include "oglbackground.h"

_MIGRENDER_BEGIN

struct OglDeferredRender
{
	double distance;
	const CObj* obj;
	const OglObject* oglObject;
	CMatrix tm;
};

class OglRender
{
private:
	// singleton instance
	static OglRender* _theRender;

	std::map<const CBaseObj*, std::unique_ptr<OglObject>> _mapObjects;
	std::map<std::string, GLuint> _mapTextures;
	std::map<const CObj*, CPt> _mapCenterPoints;

	std::map<OglProgramType, std::unique_ptr<OglProgramBase>> _thePrograms;
	OglMainProgram* _mainProgram;

	OglBackground _theBackground;

	// rendering variables
	CMatrix _tmView, _tmPerspective;
	CPt _viewPos;
	std::vector<OglDeferredRender> _deferredRenders;

private:
	void LoadViewPerspectiveMatrices(const CCamera& camera);

	void ProcessPolygon(const CModel& model, const CPolygon& poly, const CMatrix& itm);
	void ProcessSphere(const CModel& model, const CSphere& sphere, const CMatrix& itm);
	void ProcessLight(const CModel& model, const CLight& lit, const CMatrix& itm);
	void ProcessGroup(const CModel& model, const CGrp& grp, const CMatrix& itm);

	bool FindAndUpdateObject(const CModel& model, const CGrp& grp, const CMatrix& itm, const CBaseObj* pobj);

	void RenderLights(const CModel& model, const CGrp& grp, const CMatrix& itm, int& lightIndex);
	void RenderAllLights(const CModel& model);
	void RenderGroup(const CModel& model, const CGrp& grp, const CMatrix& itm);
	void RenderSuperGroup(const CModel& model);
	void RenderDeferredObjects(const CModel& model);

public:
	static OglRender* GetInstance(void);
	static OglProgramBase* GetProgram(OglProgramType programType);
	static OglMainProgram& GetMainProgram(void);

	static GLuint GetTexture(const std::string& name);
	static void AddTexture(const std::string& name, GLuint textureId);

public:
	OglRender(void);
	virtual ~OglRender(void);

	void Init(void);
	void Term(void);

	const CMatrix& GetViewMatrix(void) { return _tmView; }
	const CMatrix& GetPerspectiveMatrix(void) { return _tmPerspective; }

	void BuildModel(const CModel& model);
	bool UpdateObject(const CModel& model, const CBaseObj* pobj);
	void ResetModel(bool resetTextures = false);

	void Render(const CModel& model);
};

_MIGRENDER_END
