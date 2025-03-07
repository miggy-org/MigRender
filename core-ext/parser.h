// parser.h - defines a parser that generates models from scripts
//

#pragma once

#include <string>
#include <map>
#include <vector>
#include <istream>

#include "../core/migexcept.h"
#include "../core/model.h"
#include "../core/animmanage.h"

_MIGRENDER_BEGIN

// parsing exceptions
class parse_exception : public mig_exception
{
private:
	std::string _help;
	std::string _script;
	int _line;

public:
	using _Mybase = mig_exception;

	explicit parse_exception(const std::string& _Message) : _Mybase(_Message.c_str()), _line(0) {}
	explicit parse_exception(const std::string& _Message, const std::string& _Help)
		: _Mybase(_Message.c_str()), _help(_Help), _line(0) {}
	explicit parse_exception(const std::string& _Message, const std::string& _Script, int _Line)
		: _Mybase(_Message.c_str()), _script(_Script), _line(_Line) {}
	explicit parse_exception(const std::string& _Message, const std::string& _Help, const std::string& _Script, int _Line)
		: _Mybase(_Message.c_str()), _help(_Help), _script(_Script), _line(_Line) {}

	const std::string& help() const _NOEXCEPT
	{
		return _help;
	}

	const std::string& script() const _NOEXCEPT
	{
		return _script;
	}

	int line() const _NOEXCEPT
	{
		return _line;
	}
};

template <class T>
class CArray
{
private:
	std::vector<T> list;

public:
	void Add(const T& newItem) { list.push_back(newItem); }
	void Clear(void) { list.clear(); }

	T* ToArray(void) { return (list.size() > 0 ? &list[0] : NULL); }
	int GetSize(void) { return (int) list.size(); }
};

//-----------------------------------------------------------------------------
// CParser - a parser class (not thread safe)
//-----------------------------------------------------------------------------

#define DECLARE_MAP(type, name) \
std::map<std::string, type> name; \
template <> std::map<std::string, type>& GetVarMap() { return name; }

class CParser
{
protected:
	CModel* m_pmodel;
	CAnimManager* m_panimmanager;
	double m_aspect;
	bool m_aspectIsFixed;  // indicates aspect ratio was computed from render target and cannot be changed

	std::string m_script;
	std::string m_envpath;
	int m_line;

	std::string m_returnVarName;
	std::string m_scriptReturnVarName;

	bool m_helpActive;
	bool m_echoActive;

protected:
	template <typename T> std::map<std::string, T>& GetVarMap();
	template <typename T> void CreateNewVar(const std::string& name);
	template <typename T> bool InVarMap(const std::string& name);
	template <typename T> T& GetFromVarMap(const std::string& name);
	template <typename T> void AssignReturnValue(T value);
	template <typename T> bool CopyEnvVarToNewParser(const std::string& name, CParser& newParser);
	template <typename T> bool CopyReturnValueFromOtherParser(CParser& other);

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
	void ThrowParseException(const std::string& msg, const std::string& token);
	void ThrowParseException(const std::string& msg, const std::string& token, const std::string& help);
	void CheckArgs(const std::vector<std::string>& args, size_t minCount, const std::string& cmd, bool canEcho, const std::string& help);

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
	void ParseMatrixCommand(CMatrix& matrix, const std::vector<std::string>& comps, std::vector<std::string>& args);
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
	static std::string ObjectFlagsToString(dword flags);
	static std::string PolyCurveFlagsToString(dword flags);
	static std::string TexturingFlagsToString(dword flags);
	static std::string RenderFlagsToString(dword flags);

	// static enum parsers
	template <typename T> static T ParseEnum(const std::string& enumStr);
	template <typename T> static std::string EnumToString(T value);

	// static object parsers
	template <typename T> static std::string Stringify(const T& obj);

public:
	CParser();
	CParser(CModel* pmodel);
	CParser(CModel* pmodel, CAnimManager* panimmanager);
	CParser(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo);
	virtual ~CParser();

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
};

_MIGRENDER_END
