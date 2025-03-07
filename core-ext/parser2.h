// parser2.h - experimental
//

#pragma once

#include <string>
#include <map>
#include <vector>
#include <istream>
#include <functional>

#include "../core/migexcept.h"
#include "../core/model.h"
#include "../core/animmanage.h"

#include "parser.h"

_MIGRENDER_BEGIN

//-----------------------------------------------------------------------------
// CParser2 - a parser class (not thread safe)
//-----------------------------------------------------------------------------

class CParserCommand
{
protected:
	std::string m_help;

	std::vector<CParserCommand> m_subCommands;

	std::function<void(const std::string& cmd, std::vector<std::string>& args)> m_exec;

public:
	CParserCommand(void);
	CParserCommand(std::function<void(const std::string& cmd, std::vector<std::string>& args)> exec);

	void Execute(const std::string& cmd, std::vector<std::string>& args);
};

template <class T>
class CParserTypeCommand
{
	std::string m_help;

	std::function<void(T& obj, const std::string& cmd, std::vector<std::string>& args)> m_exec;

public:
	CParserTypeCommand(void);
	CParserTypeCommand(T& obj, std::function<void(const std::string& cmd, std::vector<std::string>& args)> exec);

	void Execute(T& obj, const std::string& cmd, std::vector<std::string>& args);
};

class CParser2
{
protected:
	CModel* m_pmodel;
	CAnimManager* m_panimmanager;
	double m_aspect;
	bool m_aspectIsFixed;  // indicates aspect ratio was computed from render target and cannot be changed

	std::string m_script;
	std::string m_envpath;
	std::string m_serr;
	int m_line;

	std::string m_returnVarName;
	std::string m_scriptReturnVarName;

	std::map<std::string, CParserCommand> m_cmdHandlers;

protected:
	template <typename T> std::map<std::string, T>& GetVarMap();
	template <typename T> void CreateNewVar(const std::string& name);
	template <typename T> bool InVarMap(const std::string& name);
	template <typename T> T& GetFromVarMap(const std::string& name);
	template <typename T> void AssignReturnValue(T value);
	template <typename T> bool CopyEnvVarToNewParser(const std::string& name, CParser2& newParser);
	template <typename T> bool CopyReturnValueFromOtherParser(CParser2& other);

	DECLARE_MAP(int, mapInts);
	DECLARE_MAP(double, mapDoubles);
	DECLARE_MAP(bool, mapBooleans);
	DECLARE_MAP(std::string, mapStrings);
	DECLARE_MAP(COLOR, mapColors);
	DECLARE_MAP(UVC, mapUVCs);
	DECLARE_MAP(CPt, mapPts);
	DECLARE_MAP(CUnitVector, mapUnitVectors);
	DECLARE_MAP(CMatrix, mapMatrices);
	DECLARE_MAP(CAnimRecord, mapAnimRecords);

	DECLARE_MAP(CGrp*, mapGroups);
	DECLARE_MAP(CSphere*, mapSpheres);
	DECLARE_MAP(CPolygon*, mapPolygons);
	DECLARE_MAP(CDirLight*, mapDirLights);
	DECLARE_MAP(CPtLight*, mapPtLights);
	DECLARE_MAP(CSpotLight*, mapSpotLights);

	DECLARE_MAP(CArray<int>, mapIntArrays);
	DECLARE_MAP(CArray<double>, mapDoubleArrays);
	DECLARE_MAP(CArray<std::string>, mapStringArrays);
	DECLARE_MAP(CArray<COLOR>, mapColorArrays);
	DECLARE_MAP(CArray<UVC>, mapUVCArrays);
	DECLARE_MAP(CArray<CPt>, mapPtArrays);
	DECLARE_MAP(CArray<CUnitVector>, mapUnitVectorArrays);

protected:
	template <typename T> T ParseObject(const std::string& token);
	template <typename T> T ParseObjectCached(const std::string& name);
	template <typename T> T ParseObjectInvertible(const std::string& name);
	template <typename T> std::unique_ptr<T> ParseObjectNullable(const std::string& token);
	template <typename T> const CArray<T>& ParseArray(const std::string& token);
	template <typename T> bool ParseCreateCommand(const std::string& typeName, const std::string& name, const std::string& type);
	template <typename T> bool ParseArrayCommand(const std::string& root, const std::string& command, std::vector<std::string>& args);
	template <typename T> bool ParsePrimitiveCommand(const std::string& root, const std::string& command, std::vector<std::string>& args);

protected:
	void InitCommandHandlers(void);

	std::string GenerateObjectNamespace(const CGrp* parent, const CBaseObj* pobj) const;
	std::string GenerateObjectNamespace(const CGrp* parent, const std::string& name);
	CBaseObj* DrillDown(CGrp* parent, std::string& next, std::vector<std::string>& comp);
	CBaseObj* DrillDown(const std::string& name, ObjType expectedType = ObjType::None);
	CBaseObj* FindObjInEnvVars(const std::string& name);

	bool ParseCreateCommand(std::vector<std::string>& args);
	bool ParsePrimitiveCommand(const std::string& root, std::vector<std::string>& comps, std::vector<std::string>& args);
	bool ParseArrayCommand(const std::string& root, std::vector<std::string>& comps, std::vector<std::string>& args);

	void ParseScriptCommand(std::vector<std::string>& args);
	void ParseMatrixCommand(CMatrix& matrix, const std::string& command, std::vector<std::string>& args);
	void ParseCameraCommand(std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseBgCommand(std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseImageMapCommand(std::vector<std::string>& comps, std::vector<std::string>& args);
	bool ParseImagesCommand(const std::string& root, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseAnimRecordValues(CAnimRecord& record, const std::string& beginStr, const std::string& endStr);
	void ParseAnimationCommand(std::vector<std::string>& comps, std::vector<std::string>& args);

	void ParseBaseObjectCommand(CBaseObj* pobj, const std::string& command, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseObjectCommand(CObj* pobj, const std::string& command, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseGroupCommand(CGrp* pgrp, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseLightCommand(CLight* plit, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParseSphereCommand(CSphere* psphere, std::vector<std::string>& comps, std::vector<std::string>& args);
	void ParsePolygonCommand(CPolygon* ppoly, std::vector<std::string>& comps, std::vector<std::string>& args);

	bool ParseCommandStringInner(const std::string& line);

public:
	// static flag parsers
	static dword ParseObjectFlags(const std::string& token);
	static dword ParsePolyCurveFlags(const std::string& token);
	static dword ParseTexturingFlags(const std::string& token);
	static dword ParseRenderFlags(const std::string& token);

	// static enum parsers
	template <typename T> static T ParseEnum(const std::string& enumStr);

public:
	CParser2();
	CParser2(CModel* pmodel);
	CParser2(CModel* pmodel, CAnimManager* panimmanager);
	CParser2(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo);
	virtual ~CParser2();

	// pass REND_INFO if you intend to render and ignore script specified camera aspect ratio
	bool Init(CModel* pmodel);
	bool Init(CModel* pmodel, CAnimManager* panimmanager);
	bool Init(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo);
	void SetEnvPath(const std::string& envpath);

	template <typename T> bool SetEnvVar(const std::string& name, const T& value);
	template <typename T> bool GetEnvVar(const std::string& name, T& value);
	template <typename T> bool SetEnvVarModelRef(const std::string& name, T* value);
	template <typename T> bool GetEnvVarModelRef(const std::string& name, T*& value);

	bool ParseCommandString(const std::string& line);
	bool ParseCommandScript(std::istream& stream);
	bool ParseCommandScript(const std::string& path);
	bool ParseCommandScriptString(const std::string& script);

	const std::string& GetError() const;
};

_MIGRENDER_END
