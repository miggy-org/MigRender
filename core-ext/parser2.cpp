// parser2.cpp - experimental
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <string.h>

#include "../core/package.h"
#include "parser2.h"
#include "jsonio.h"

using namespace std;
using namespace MigRender;

#ifndef _MAX_PATH
#define _MAX_PATH 260
#endif  // _MAX_PATH

#pragma warning(disable : 4996)

static const char* NESTED_DELIM = ".";
static const char* COMMA_DELIM = " \t,";
static const char* FLAG_DELIM = " \t()|";

static const int COMMENT_PREFIX = '#';

inline void ThrowParseException(const string& message, const string& token)
{
	throw parse_exception(message + " [" + token + "]");
}

static void ValidateArgCount(vector<string> args, size_t minCount, string error)
{
	if (args.size() < minCount)
		ThrowParseException("Arg count failed, expected " + to_string(minCount), error);
}

// not thread safe
static char* GetStringBuffer(const string& token)
{
	if (token.length() >= _MAX_PATH - 1)
		ThrowParseException("Token string too large", token);

	static char buffer[_MAX_PATH];
	strncpy(buffer, token.c_str(), token.length() + 1);
	return buffer;
}

static char* GetInnerString(const string& token, char begin, char end)
{
	size_t lastindex = token.length();
	if (token[0] != begin || token[lastindex - 1] != end)
		ThrowParseException("Error parsing inner string", token);

	return GetStringBuffer(token.substr(1, lastindex - 2));
}

static bool IsDelimCharacter(char c)
{
	return (c == 0 || c == ' ' || c == ',' || c == '\t');
}

static int SplitRootString(const string& line, vector<string>& out)
{
	if (line.length() >= _MAX_PATH - 1)
		ThrowParseException("Line string too large", line);

	char buffer[_MAX_PATH];
	strncpy(buffer, line.c_str(), line.length() + 1);

	// filter out comments
	char* comment = strchr(buffer, COMMENT_PREFIX);
	if (comment != NULL)
		*comment = 0;

	int length = (int)strlen(buffer);
	char* start = buffer;
	char* end = start;
	while (end - buffer < length)
	{
		if (start == end)
		{
			if (*start == '[')
			{
				end = strchr(start + 1, ']');
				if (end == NULL)
					ThrowParseException("Format error", line);
				end++;
				if (!IsDelimCharacter (*end))
					ThrowParseException("Format error", line);
				*end = 0;
				if (strchr(start + 1, '[') != NULL)
					ThrowParseException("Format error", line);
				out.push_back(string(start));

				end++;
				start = end;
				continue;
			}
			else if (*start == '(')
			{
				end = strchr(start + 1, ')');
				if (end == NULL)
					ThrowParseException("Format error", line);
				end++;
				if (!IsDelimCharacter(*end))
					ThrowParseException("Format error", line);
				*end = 0;
				if (strchr(start + 1, '(') != NULL)
					ThrowParseException("Format error", line);
				out.push_back(string(start));

				end++;
				start = end;
				continue;
			}
			else if (*start == '\"')
			{
				end = strchr(start + 1, '\"');
				if (end == NULL)
					ThrowParseException("Format error", line);
				end++;
				if (!IsDelimCharacter(*end))
					ThrowParseException("Format error", line);
				*end = 0;
				out.push_back(string(start));

				end++;
				start = end;
				continue;
			}
		}

		if (IsDelimCharacter(*end))
		{
			*end = 0;
			if (strlen(start) > 0)
				out.push_back(string(start));

			end++;
			start = end;
		}
		else
			end++;
	}
	if (end > start)
		out.push_back(string(start));

	return (int) out.size();
}

static int SplitComponents(const string& token, const char* delim, vector<string>& out)
{
	if (token.length() >= _MAX_PATH - 1)
		ThrowParseException("Token string too large", token);

	char buffer[_MAX_PATH];
	strncpy(buffer, token.c_str(), token.length() + 1);

	char* item = strtok(buffer, delim);
	while (item != NULL)
	{
		out.push_back(string(item));
		item = strtok(NULL, delim);
	}

	return (int) out.size();
}

static int SplitNestedComponents(const string& token, vector<string>& out)
{
	return SplitComponents(token, NESTED_DELIM, out);
}

//-----------------------------------------------------------------------------
// CParserCommand
//-----------------------------------------------------------------------------

CParserCommand::CParserCommand(void)
	: m_exec(nullptr)
{
}

CParserCommand::CParserCommand(std::function<void(const std::string& cmd, std::vector<std::string>& args)> exec)
	: m_exec(exec)
{
}

void CParserCommand::Execute(const std::string& cmd, std::vector<std::string>& args)
{
	m_exec(cmd, args);
}

template <class T>
CParserTypeCommand<T>::CParserTypeCommand(void)
	: m_exec(nullptr)
{
}

template <class T>
CParserTypeCommand<T>::CParserTypeCommand(T& obj, std::function<void(const std::string& cmd, std::vector<std::string>& args)> exec)
	: m_exec(exec)
{
}

template <class T>
void CParserTypeCommand<T>::Execute(T& obj, const std::string& cmd, std::vector<std::string>& args)
{
	m_exec(obj, cmd, args);
}

//-----------------------------------------------------------------------------
// CParser2
//-----------------------------------------------------------------------------

CParser2::CParser2()
{
	m_pmodel = NULL;
	m_panimmanager = NULL;
	m_aspect = 1.333;
	m_aspectIsFixed = false;
	m_line = 0;
}

CParser2::CParser2(CModel* pmodel)
{
	Init(pmodel);
}

CParser2::CParser2(CModel* pmodel, CAnimManager* panimmanager)
{
	Init(pmodel, panimmanager);
}

CParser2::CParser2(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo)
{
	Init(pmodel, panimmanager, rinfo);
}

CParser2::~CParser2()
{
	m_pmodel = NULL;
}

bool CParser2::Init(CModel* pmodel)
{
	return Init(pmodel, NULL);
}

bool CParser2::Init(CModel* pmodel, CAnimManager* panimmanager)
{
	InitCommandHandlers();

	m_pmodel = pmodel;
	if (m_pmodel != NULL)
		SetEnvVarModelRef<CGrp>("super", &m_pmodel->GetSuperGroup());
	m_panimmanager = panimmanager;
	m_aspect = 1.333;
	m_aspectIsFixed = false;
	return true;
}

bool CParser2::Init(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo)
{
	InitCommandHandlers();

	if (!Init(pmodel, panimmanager))
		return false;
	m_aspect = (rinfo.height > 0 ? rinfo.width / (double)rinfo.height : 0);
	m_aspectIsFixed = true;
	return true;
}

void CParser2::SetEnvPath(const std::string& envpath)
{
	m_envpath = envpath;
}

// getters for the variable maps
//

template <typename T>
map<string, T>& CParser2::GetVarMap()
{
	throw parse_exception("Unknown map type");
}

template <typename T>
bool CParser2::InVarMap(const string& name)
{
	map<string, T>& mapObjs = GetVarMap<T>();
	return (mapObjs.find(name) != mapObjs.end());
}

template <typename T>
T& CParser2::GetFromVarMap(const string& name)
{
	return GetVarMap<T>()[name];
}

template <typename T>
void CParser2::CreateNewVar(const string& name)
{
	GetVarMap<T>()[name];
}

template <> void CParser2::CreateNewVar<CGrp*>(const string& name)
{
	CGrp* newGroup = m_pmodel->GetSuperGroup().CreateSubGroup();
	GetVarMap<CGrp*>()[name] = newGroup;
}

template <> void CParser2::CreateNewVar<CSphere*>(const string& name)
{
	CSphere* newSphere = m_pmodel->GetSuperGroup().CreateSphere();
	GetVarMap<CSphere*>()[name] = newSphere;
}

template <> void CParser2::CreateNewVar<CPolygon*>(const string& name)
{
	CPolygon* newPoly = m_pmodel->GetSuperGroup().CreatePolygon();
	GetVarMap<CPolygon*>()[name] = newPoly;
}

template <> void CParser2::CreateNewVar<CDirLight*>(const string& name)
{
	CDirLight* newLight = m_pmodel->GetSuperGroup().CreateDirectionalLight();
	GetVarMap<CDirLight*>()[name] = newLight;
}

template <> void CParser2::CreateNewVar<CPtLight*>(const string& name)
{
	CPtLight* newLight = m_pmodel->GetSuperGroup().CreatePointLight();
	GetVarMap<CPtLight*>()[name] = newLight;
}

template <> void CParser2::CreateNewVar<CSpotLight*>(const string& name)
{
	CSpotLight* newLight = m_pmodel->GetSuperGroup().CreateSpotLight();
	GetVarMap<CSpotLight*>()[name] = newLight;
}

template <typename T> void CParser2::AssignReturnValue(T value)
{
	if (!m_returnVarName.empty())
	{
		//CreateNewVar<T>(m_returnVarName);
		GetVarMap<T>()[m_returnVarName] = value;

		m_returnVarName.clear();
	}
}

template <typename T>
bool CParser2::CopyEnvVarToNewParser(const string& name, CParser2& newParser)
{
	bool returnValue = InVarMap<T>(name);
	if (returnValue)
		newParser.SetEnvVar<T>(name, GetFromVarMap<T>(name));
	return returnValue;
}

template <typename T>
bool CParser2::CopyReturnValueFromOtherParser(CParser2& other)
{
	bool returnValue = other.InVarMap<T>(other.m_scriptReturnVarName);
	if (returnValue)
		AssignReturnValue<T>(other.GetFromVarMap<T>(other.m_scriptReturnVarName));
	return returnValue;
}

// parsers for the flags
//

static dword ParseFlags(const string& token, dword none, map<string, dword> mapFlags)
{
	dword flags = none;

	string copy = token;
	if (copy[0] == '(')
		copy = GetInnerString(copy, '(', ')');

	char* flag = strtok(GetStringBuffer(copy), FLAG_DELIM);
	while (flag != NULL)
	{
		if (mapFlags.find(flag) != mapFlags.end())
			flags |= mapFlags[flag];
		else
			ThrowParseException("Error parsing flags", token);

		flag = strtok(NULL, FLAG_DELIM);
	}

	return flags;
}

dword CParser2::ParseObjectFlags(const string& token)
{
	static map<string, dword> mapFlags = {
		{"none", OBJF_NONE},
		{"shadow", OBJF_SHADOW_RAY},
		{"refl", OBJF_REFL_RAY},
		{"refr", OBJF_REFR_RAY},
		{"caster", OBJF_SHADOW_CASTER},
		{"bbox", OBJF_USE_BBOX},
		{"invisible", OBJF_INVISIBLE},
		{"all", OBJF_ALL_RAYS}
	};
	return ParseFlags(token, OBJF_NONE, mapFlags);
}

dword CParser2::ParsePolyCurveFlags(const string& token)
{
	static map<string, dword> mapFlags = {
		{"none", PCF_NONE},
		{"newplane", PCF_NEWPLANE},
		{"cull", PCF_CULL},
		{"invisible", PCF_INVISIBLE},
		{"cw", PCF_CW},
		{"vertexnorms", PCF_VERTEXNORMS}
	};
	return ParseFlags(token, PCF_NONE, mapFlags);
}

dword CParser2::ParseTexturingFlags(const string& token)
{
	static map<string, dword> mapFlags = {
		{"none", TXTF_NONE},
		{"enabled", TXTF_ENABLED},
		{"rgb", TXTF_RGB},
		{"alpha", TXTF_ALPHA},
		{"invert", TXTF_INVERT},
		{"bump", TXTF_IS_BUMP},
		{"invertalpha", TXTF_INVERT_ALPHA}
	};
	return ParseFlags(token, TXTF_NONE, mapFlags);
}

dword CParser2::ParseRenderFlags(const string& token)
{
	static map<string, dword> mapFlags = {
		{"none", REND_NONE},
		{"reflect", REND_AUTO_REFLECT},
		{"refract", REND_AUTO_REFRACT},
		{"shadows", REND_AUTO_SHADOWS},
		{"softshadows", REND_SOFT_SHADOWS},
		{"all", REND_ALL}
	};
	return ParseFlags(token, REND_NONE, mapFlags);
}

template <typename T>
static T LookupEnum(const string& enumStr, map<string, T>& mapFlags, T defIfEmpty)
{
	if (enumStr.empty())
		return defIfEmpty;
	if (mapFlags.find(enumStr) == mapFlags.end())
		ThrowParseException("Unrecognized enum", enumStr);
	return mapFlags[enumStr];
}

template <typename T>
T CParser2::ParseEnum(const string& enumStr)
{
	ThrowParseException("Unknown enum type", enumStr);
}

template <> SuperSample CParser2::ParseEnum(const string& token)
{
	static map<string, SuperSample> mapFlags = {
		{"1x", SuperSample::X1},
		{"5x", SuperSample::X5},
		{"9x", SuperSample::X9},
		{"edge", SuperSample::Edge},
		{"object", SuperSample::Object}
	};
	return LookupEnum(token, mapFlags, SuperSample::X1);
}

template <> ImageResize CParser2::ParseEnum(const string& token)
{
	static map<string, ImageResize> mapFlags = {
		{"stretch", ImageResize::Stretch},
		{"fit", ImageResize::ScaleToFit},
		{"fill", ImageResize::ScaleToFill}
	};
	return LookupEnum(token, mapFlags, ImageResize::Stretch);
}

template <> ImageFormat CParser2::ParseEnum(const string& token)
{
	static map<string, ImageFormat> mapFlags = {
		{"none", ImageFormat::None},
		{"rgba", ImageFormat::RGBA},
		{"bgra", ImageFormat::BGRA},
		{"rgb", ImageFormat::RGB},
		{"bgr", ImageFormat::BGR},
		{"greyscale", ImageFormat::GreyScale},
		{"bump", ImageFormat::Bump}
	};
	return LookupEnum(token, mapFlags, ImageFormat::None);
}

template <> TextureFilter CParser2::ParseEnum(const string& token)
{
	static map<string, TextureFilter> mapFlags = {
		{"none", TextureFilter::None},
		{"nearest", TextureFilter::Nearest},
		{"bilinear", TextureFilter::Bilinear}
	};
	return LookupEnum(token, mapFlags, TextureFilter::None);
}

template <> TextureMapType CParser2::ParseEnum(const string& token)
{
	static map<string, TextureMapType> mapFlags = {
		{"none", TextureMapType::None},
		{"diffuse", TextureMapType::Diffuse},
		{"specular", TextureMapType::Specular},
		{"reflection", TextureMapType::Reflection},
		{"refraction", TextureMapType::Refraction},
		{"glow", TextureMapType::Glow},
		{"transparency", TextureMapType::Transparency},
		{"bump", TextureMapType::Bump}
	};
	return LookupEnum(token, mapFlags, TextureMapType::None);
}

template <> TextureMapOp CParser2::ParseEnum(const string& token)
{
	static map<string, TextureMapOp> mapFlags = {
		{"multiply", TextureMapOp::Multiply},
		{"add", TextureMapOp::Add},
		{"subtract", TextureMapOp::Subtract},
		{"blend", TextureMapOp::Blend}
	};
	return LookupEnum(token, mapFlags, TextureMapOp::Multiply);
}

template <> TextureMapWrapType CParser2::ParseEnum(const string& token)
{
	static map<string, TextureMapWrapType> mapFlags = {
		{"none", TextureMapWrapType::None},
		{"spherical", TextureMapWrapType::Spherical},
		{"elliptical", TextureMapWrapType::Elliptical},
		{"projection", TextureMapWrapType::Projection},
		{"extrusion", TextureMapWrapType::Extrusion}
	};
	return LookupEnum(token, mapFlags, TextureMapWrapType::None);
}

template <> AnimType CParser2::ParseEnum(const string& token)
{
	static map<string, AnimType> mapFlags = {
		{"matrix_translate", AnimType::Translate},
		{"matrix_scale", AnimType::Scale},
		{"matrix_rotate_x", AnimType::RotateX},
		{"matrix_rotate_y", AnimType::RotateY},
		{"matrix_rotate_z", AnimType::RotateZ},
		{"color_diffuse", AnimType::ColorDiffuse},
		{"color_specular", AnimType::ColorSpecular},
		{"color_reflection", AnimType::ColorReflection},
		{"color_refraction", AnimType::ColorRefraction},
		{"color_glow", AnimType::ColorGlow},
		{"index_refraction", AnimType::IndexRefraction},
		{"bg_color_north", AnimType::BgColorNorth},
		{"bg_color_equator", AnimType::BgColorEquator},
		{"bg_color_south", AnimType::BgColorSouth},
		{"camera_dist", AnimType::CameraDist},
		{"camera_ulen", AnimType::CameraULen},
		{"camera_vlen", AnimType::CameraVLen},
		{"light_color", AnimType::LightColor},
		{"light_hlit", AnimType::LightHLit},
		{"light_sscale", AnimType::LightSScale},
		{"light_full_distance", AnimType::LightFullDistance},
		{"light_drop_distance", AnimType::LightDropDistance},
		{"model_ambient", AnimType::ModelAmbient},
		{"model_fade", AnimType::ModelFade},
		{"model_fog", AnimType::ModelFog},
		{"model_fog_near", AnimType::ModelFogNear},
		{"model_fog_far", AnimType::ModelFogFar},
		{"none", AnimType::None}
	};
	return LookupEnum(token, mapFlags, AnimType::None);
}

template <> AnimInterpolation CParser2::ParseEnum(const string& token)
{
	static map<string, AnimInterpolation> mapFlags = {
		{"spline", AnimInterpolation::Spline},
		{"spline2", AnimInterpolation::Spline2},
		{"linear", AnimInterpolation::Linear}
	};
	return LookupEnum(token, mapFlags, AnimInterpolation::Linear);
}

template <> AnimOperation CParser2::ParseEnum(const string& token)
{
	static map<string, AnimOperation> mapFlags = {
		{"multiply", AnimOperation::Multiply},
		{"sum", AnimOperation::Sum},
		{"replace", AnimOperation::Replace}
	};
	return LookupEnum(token, mapFlags, AnimOperation::Replace);
}

// parsers for the basic object types, not using the maps
//

template <typename T>
T CParser2::ParseObject(const string& token)
{
	ThrowParseException("Unknown parsing type", token);
}

#include <algorithm>

template <> int CParser2::ParseObject(const string& token)
{
	//if (!std::all_of(token.begin(), token.end(), ::isdigit))
	//	ThrowParseException("Invalid integer string", token);
	//return atoi(token.c_str());
	char* p;
	int i = strtol(token.c_str(), &p, 10);
	if (*p != 0)
		ThrowParseException("Invalid int string", token);
	return i;
}

template <> double CParser2::ParseObject(const string& token)
{
	//return atof(token.c_str());
	char* p;
	double d = strtod(token.c_str(), &p);
	if (*p != 0)
		ThrowParseException("Invalid double string", token);
	return d;
}

template <> bool CParser2::ParseObject(const string& token)
{
	if (token == "true")
		return true;
	else if (token == "false")
		return false;
	throw parse_exception("Error parsing bool: " + token);
}

template <> string CParser2::ParseObject(const string& token)
{
	size_t lastindex = token.size();
	if (token[0] != '\"' || token[lastindex - 1] != '\"')
		return token;

	return token.substr(1, lastindex - 2);
}

template <> SuperSample CParser2::ParseObject(const string& token)
{
	return ParseEnum<SuperSample>(token);
}

template <> ImageResize CParser2::ParseObject(const string& token)
{
	return ParseEnum<ImageResize>(token);
}

template <> ImageFormat CParser2::ParseObject(const string& token)
{
	return ParseEnum<ImageFormat>(token);
}

template <> TextureFilter CParser2::ParseObject(const string& token)
{
	return ParseEnum<TextureFilter>(token);
}

template <> TextureMapType CParser2::ParseObject(const string& token)
{
	return ParseEnum<TextureMapType>(token);
}

template <> TextureMapOp CParser2::ParseObject(const string& token)
{
	return ParseEnum<TextureMapOp>(token);
}

template <> TextureMapWrapType CParser2::ParseObject(const string& token)
{
	return ParseEnum<TextureMapWrapType>(token);
}

template <> COLOR CParser2::ParseObject(const string& token)
{
	char* red = strtok(GetInnerString(token, '(', ')'), COMMA_DELIM);
	if (red == NULL)
		ThrowParseException("Error parsing COLOR", token);
	char* grn = strtok(NULL, COMMA_DELIM);
	if (grn == NULL)
		ThrowParseException("Error parsing COLOR", token);
	char* blu = strtok(NULL, COMMA_DELIM);
	if (blu == NULL)
		ThrowParseException("Error parsing COLOR", token);
	char* alp = strtok(NULL, COMMA_DELIM);

	double r = ParseObjectInvertible<double>(red);
	double g = ParseObjectInvertible<double>(grn);
	double b = ParseObjectInvertible<double>(blu);
	double a = (alp != NULL ? ParseObjectInvertible<double>(alp) : 1);
	return COLOR(r, g, b, a);
}

template <> UVC CParser2::ParseObject(const string& token)
{
	char* v1 = strtok(GetInnerString(token, '(', ')'), COMMA_DELIM);
	if (v1 == NULL)
		ThrowParseException("Error parsing UVC", token);
	char* v2 = strtok(NULL, COMMA_DELIM);
	if (v2 == NULL)
		ThrowParseException("Error parsing UVC", token);

	double u = ParseObjectInvertible<double>(v1);
	double v = ParseObjectInvertible<double>(v2);
	return UVC(u, v);
}

template <> CPt CParser2::ParseObject(const string& token)
{
	char* next = NULL;
	char* v1 = strtok(GetInnerString(token, '(', ')'), COMMA_DELIM);
	if (v1 == NULL)
		ThrowParseException("Error parsing CPt", token);
	char* v2 = strtok(NULL, COMMA_DELIM);
	if (v2 == NULL)
		ThrowParseException("Error parsing CPt", token);
	char* v3 = strtok(NULL, COMMA_DELIM);
	if (v3 == NULL)
		ThrowParseException("Error parsing CPt", token);

	double x = ParseObjectInvertible<double>(v1);
	double y = ParseObjectInvertible<double>(v2);
	double z = ParseObjectInvertible<double>(v3);
	return CPt(x, y, z);
}

template <> CUnitVector CParser2::ParseObject(const string& token)
{
	CPt pt = ParseObject<CPt>(token);
	return CUnitVector(pt);
}

// parsers for the basic object types, using the maps
//

template <typename T>
T CParser2::ParseObjectCached(const string& name)
{
	map<string, T>& mapCache = GetVarMap<T>();
	if (mapCache.find(name) != mapCache.end())
		return mapCache[name];
	else
		return ParseObject<T>(name);
}

template <> CGrp* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CGrp*) DrillDown(name, ObjType::Group);
	else
	{
		CGrp* pgrp = NULL;

		vector<string> comp;
		if (SplitNestedComponents(name, comp) > 0)
		{
			string root = comp[0];
			comp.erase(comp.begin());

			pgrp = GetFromVarMap<CGrp*>(root);
			if (pgrp == NULL)
				ThrowParseException("Error parsing object", name);

			if (!comp.empty())
			{
				root = comp[0];
				comp.erase(comp.begin());

				CBaseObj* pobj = DrillDown(pgrp, root, comp);
				if (pobj == NULL || pobj->GetType() != ObjType::Group)
					ThrowParseException("Error parsing object", name);
				pgrp = (CGrp*)pobj;
			}
		}
		return pgrp;
	}
}

template <> CDirLight* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CDirLight*)DrillDown(name, ObjType::DirLight);
	else
		return GetFromVarMap<CDirLight*>(name);
}

template <> CPtLight* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CPtLight*)DrillDown(name, ObjType::PtLight);
	else
		return GetFromVarMap<CPtLight*>(name);
}

template <> CSpotLight* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CSpotLight*)DrillDown(name, ObjType::SpotLight);
	else
		return GetFromVarMap<CSpotLight*>(name);
}

template <> CSphere* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CSphere*)DrillDown(name, ObjType::Sphere);
	else
		return GetFromVarMap<CSphere*>(name);
}

template <> CPolygon* CParser2::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CPolygon*)DrillDown(name, ObjType::Polygon);
	else
		return GetFromVarMap<CPolygon*>(name);
}

template <> CBaseObj* CParser2::ParseObjectCached(const string& name)
{
	// try drill down first
	if (name[0] == '.')
		return DrillDown(name, ObjType::None);

	// try direct matches
	CGrp* pgrp = GetFromVarMap<CGrp*>(name);
	if (pgrp != NULL)
		return pgrp;
	CPolygon* ppoly = GetFromVarMap<CPolygon*>(name);
	if (ppoly != NULL)
		return ppoly;
	CSphere* psphere = GetFromVarMap<CSphere*>(name);
	if (psphere != NULL)
		return psphere;
	CDirLight* pdirlight = GetFromVarMap<CDirLight*>(name);
	if (pdirlight != NULL)
		return pdirlight;
	CPtLight* pptlight = GetFromVarMap<CPtLight*>(name);
	if (pptlight != NULL)
		return pptlight;
	CSpotLight* pspotlight = GetFromVarMap<CSpotLight*>(name);
	if (pspotlight != NULL)
		return pspotlight;

	// is there more than one element in the name
	vector<string> comp;
	if (SplitNestedComponents(name, comp) > 1)
	{
		string root = comp[0];
		comp.erase(comp.begin());

		// first element must be a group
		CGrp* pgrp = GetFromVarMap<CGrp*>(root);
		if (pgrp == NULL)
			ThrowParseException("Error parsing object", name);

		if (!comp.empty())
		{
			root = comp[0];
			comp.erase(comp.begin());

			// drill down into the final node
			CBaseObj* pobj = DrillDown(pgrp, root, comp);
			if (pobj == NULL)
				ThrowParseException("Error parsing object", name);
			return pobj;
		}
		return pgrp;
	}
	throw parse_exception("Error parsing object: " + name);
}

template <typename T>
T CParser2::ParseObjectInvertible(const string& name)
{
	bool doInvert = false;
	string lname = name;
	if (lname[0] == '-') {
		doInvert = true;
		lname = lname.substr(1);
	}

	map<string, T>& mapCache = GetVarMap<T>();
	if (mapCache.find(lname) != mapCache.end())
		return (doInvert ? -mapCache[lname] : mapCache[lname]);
	else
		return ParseObject<T>(name);
}

template <typename T>
unique_ptr<T> CParser2::ParseObjectNullable(const string& token)
{
	if (token == "null")
		return NULL;
	try
	{
		return make_unique<T>(ParseObjectCached<T>(token));
	}
	catch (const parse_exception&)
	{
		// this is ok, just means the variable wasn't set to use NULL
		return NULL;
	}
}

template <typename T>
const CArray<T>& CParser2::ParseArray(const string& token)
{
	static CArray<T> _objArray;

	map<string, CArray<T>>& mapArrays = GetVarMap<CArray<T>>();
	if (mapArrays.find(token) != mapArrays.end())
		return mapArrays[token];

	_objArray.Clear();
	if (token != "null")
	{
		vector<string> elements;
		SplitRootString(GetInnerString(token, '[', ']'), elements);

		_objArray.Clear();
		for (const auto& element : elements)
			_objArray.Add(ParseObjectCached<T>(element));
	}
	return _objArray;
}

string CParser2::GenerateObjectNamespace(const CGrp* parent, const CBaseObj* pobj) const
{
	for (size_t i = 0; i < parent->GetSubGroups().size(); i++)
	{
		const CGrp* nextgrp = parent->GetSubGroups().at(i).get();
		if (pobj == nextgrp)
			return GroupPrefixName + to_string(i + 1);

		string subName = GenerateObjectNamespace(nextgrp, pobj);
		if (!subName.empty())
			return GroupPrefixName + to_string(i + 1) + "." + subName;
	}
	for (size_t i = 0; i < parent->GetObjects().size(); i++)
	{
		const CObj* nextobj = parent->GetObjects().at(i).get();
		if (pobj == nextobj)
			return ObjectPrefixName + to_string(i + 1);
	}
	for (size_t i = 0; i < parent->GetLights().size(); i++)
	{
		const CLight* nextobj = parent->GetLights().at(i).get();
		if (pobj == nextobj)
			return LightPrefixName + to_string(i + 1);
	}
	return "";
}

string CParser2::GenerateObjectNamespace(const CGrp* parent, const string& name)
{
	CBaseObj* pobj = ParseObjectCached<CBaseObj*>(name);
	return GenerateObjectNamespace(&m_pmodel->GetSuperGroup(), pobj);
}

static size_t ExtractIndexFromName(const string& name)
{
	string indexStr = name.substr(3);
	return atoi(indexStr.c_str());
}

CBaseObj* CParser2::DrillDown(CGrp* parent, string& next, vector<string>& comp)
{
	for (const auto& grp : parent->GetSubGroups())
	{
		if (next == grp->GetName())
		{
			if (comp.empty())
			{
				next = "";
				return grp.get();
			}
			next = comp[0];
			comp.erase(comp.begin());
			return DrillDown(grp.get(), next, comp);
		}
	}
	for (const auto& obj : parent->GetObjects())
	{
		if (next == obj->GetName())
			return obj.get();
	}
	for (const auto& obj : parent->GetLights())
	{
		if (next == obj->GetName())
			return obj.get();
	}

	if (next.find(GroupPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < parent->GetSubGroups().size())
		{
			CGrp* nextgrp = parent->GetSubGroups().at(index).get();
			if (comp.empty())
			{
				next = "";
				return nextgrp;
			}
			next = comp[0];
			comp.erase(comp.begin());
			return DrillDown(nextgrp, next, comp);
		}
	}
	else if (next.find(ObjectPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < parent->GetObjects().size())
			return parent->GetObjects().at(index).get();
	}
	else if (next.find(LightPrefixName) == 0)
	{
		size_t index = ExtractIndexFromName(next) - 1;
		if (index >= 0 && index < parent->GetLights().size())
			return parent->GetLights().at(index).get();
	}

	comp.insert(comp.begin(), next);
	return parent;
}

CBaseObj* CParser2::DrillDown(const string& name, ObjType expectedType)
{
	CBaseObj* pobj = m_pmodel->GetSuperGroup().DrillDown(name);
	if (pobj == NULL)
		ThrowParseException("Unable to drill into namespace", name);
	if (expectedType != ObjType::None && pobj->GetType() != expectedType)
		ThrowParseException("Object type mismatch", name);
	return pobj;
}

CBaseObj* CParser2::FindObjInEnvVars(const string& name)
{
	if (InVarMap<CGrp*>(name))
		return GetFromVarMap<CGrp*>(name);
	if (InVarMap<CDirLight*>(name))
		return GetFromVarMap<CDirLight*>(name);
	if (InVarMap<CPtLight*>(name))
		return GetFromVarMap<CPtLight*>(name);
	if (InVarMap<CSpotLight*>(name))
		return GetFromVarMap<CSpotLight*>(name);
	if (InVarMap<CSphere*>(name))
		return GetFromVarMap<CSphere*>(name);
	if (InVarMap<CPolygon*>(name))
		return GetFromVarMap<CPolygon*>(name);
	return NULL;
}

template <typename T>
bool CParser2::ParseCreateCommand(const string& typeName, const string& name, const string& type)
{
	if (type == typeName)
	{
		CreateNewVar<T>(name);
		return true;
	}
	return false;
}

template <typename T>
bool CParser2::ParsePrimitiveCommand(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<T>(root))
	{
		T& value = GetFromVarMap<T>(root);
		if (command == "set")
		{
			ValidateArgCount(args, 1, command);
			value = ParseObjectCached<T>(args[0]);
		}
		else
			ThrowParseException("Invalid primitive command", command);

		return true;
	}
	return false;
}

template <> bool CParser2::ParsePrimitiveCommand<string>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<string>(root))
	{
		string& value = GetFromVarMap<string>(root);
		if (command == "set")
		{
			ValidateArgCount(args, 1, command);
			value = ParseObjectCached<string>(args[0]);
		}
		else if (command == "append")
		{
			ValidateArgCount(args, 1, command);
			AssignReturnValue<>(value.append(ParseObjectCached<string>(args[0])));
		}
		else if (command == "toint")
		{
			AssignReturnValue<>(atoi(value.c_str()));
		}
		else if (command == "todouble")
		{
			AssignReturnValue<>(atof(value.c_str()));
		}
		else
			ThrowParseException("Invalid string command", command);

		return true;
	}
	return false;
}

template <> bool CParser2::ParsePrimitiveCommand<CPt>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CPt>(root))
	{
		CPt& pt = GetFromVarMap<CPt>(root);
		if (command == "set")
		{
			ValidateArgCount(args, 1, command);
			pt = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "scale")
		{
			ValidateArgCount(args, 1, command);
			pt.Scale(ParseObjectInvertible<double>(args[0]));
		}
		else if (command == "invert")
		{
			pt.Invert();
		}
		else
			ThrowParseException("Invalid point command", command);

		return true;
	}
	return false;
}

template <> bool CParser2::ParsePrimitiveCommand<CUnitVector>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CUnitVector>(root))
	{
		CUnitVector& unitVector = GetFromVarMap<CUnitVector>(root);
		if (command == "set")
		{
			ValidateArgCount(args, 1, command);
			unitVector = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "point")
		{
			ValidateArgCount(args, 1, command);
			unitVector = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "normalize")
		{
			ValidateArgCount(args, 0, command);
			unitVector.Normalize();
		}
		else
			ThrowParseException("Invalid unit vector command", command);

		return true;
	}
	return false;
}

template <> bool CParser2::ParsePrimitiveCommand<CMatrix>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CMatrix>(root))
	{
		ParseMatrixCommand(GetFromVarMap<CMatrix>(root), command, args);
		return true;
	}
	return false;
}

template <> bool CParser2::ParsePrimitiveCommand<CAnimRecord>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CAnimRecord>(root))
	{
		CAnimRecord& record = GetFromVarMap<CAnimRecord>(root);

		if (command == "type")
		{
			ValidateArgCount(args, 1, command);
			record.SetAnimType(ParseEnum<AnimType>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "times")
		{
			ValidateArgCount(args, 2, command);
			record.SetTimes(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
		}
		else if (command == "reverse")
		{
			ValidateArgCount(args, 1, command);
			record.SetReverse(ParseObjectCached<bool>(args[0]));
		}
		else if (command == "applytostart")
		{
			ValidateArgCount(args, 1, command);
			record.SetApplyToStart(ParseObjectCached<bool>(args[0]));
		}
		else if (command == "operation")
		{
			ValidateArgCount(args, 1, command);
			record.SetOperation(ParseEnum<AnimOperation>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "interpolation")
		{
			ValidateArgCount(args, 1, command);
			record.SetInterpolation(ParseEnum<AnimInterpolation>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "bias")
		{
			ValidateArgCount(args, 1, command);
			record.SetBias(ParseObjectCached<double>(args[0]));
		}
		else if (command == "bias2")
		{
			ValidateArgCount(args, 1, command);
			record.SetBias2(ParseObjectCached<double>(args[0]));
		}
		else if (command == "values")
		{
			ValidateArgCount(args, 1, command);
			ParseAnimRecordValues(record, ParseObjectCached<string>(args[0]), args.size() > 1 ? ParseObjectCached<string>(args[1]) : "");
		}
		else
			ThrowParseException("Invalid animation record command", command);

		return true;
	}
	return false;
}

template <typename T>
bool CParser2::ParseArrayCommand(const string& root, const string& command, vector<string>& args)
{
	map<string, CArray<T>>& mapArrays = GetVarMap<CArray<T>>();
	if (mapArrays.find(root) != mapArrays.end())
	{
		CArray<T>& objArray = mapArrays[root];
		if (command == "set")
		{
			ValidateArgCount(args, 1, command);
			objArray = ParseArray<T>(args[0]);
		}
		else if (command == "add")
		{
			ValidateArgCount(args, 1, command);
			objArray.Add(ParseObjectCached<T>(args[0]));
		}
		else
			ThrowParseException("Invalid array command", command);

		return true;
	}

	return false;
}

bool CParser2::ParseCreateCommand(vector<string>& args)
{
	vector<string> nameType;
	if (SplitComponents(args[0], ":", nameType) > 1)
	{
		// type is included in the name, so we can create immediately and assign
		if (ParseCreateCommand<int>("int", nameType[0], nameType[1]) ||
			ParseCreateCommand<double>("double", nameType[0], nameType[1]) ||
			ParseCreateCommand<bool>("boolean", nameType[0], nameType[1]) ||
			ParseCreateCommand<string>("string", nameType[0], nameType[1]) ||
			ParseCreateCommand<COLOR>("color", nameType[0], nameType[1]) ||
			ParseCreateCommand<UVC>("uvc", nameType[0], nameType[1]) ||
			ParseCreateCommand<CPt>("pt", nameType[0], nameType[1]) ||
			ParseCreateCommand<CMatrix>("matrix", nameType[0], nameType[1]) ||
			ParseCreateCommand<CUnitVector>("unitvector", nameType[0], nameType[1]) ||
			ParseCreateCommand<CGrp*>("group", nameType[0], nameType[1]) ||
			ParseCreateCommand<CSphere*>("sphere", nameType[0], nameType[1]) ||
			ParseCreateCommand<CPolygon*>("polygon", nameType[0], nameType[1]) ||
			ParseCreateCommand<CDirLight*>("dirlight", nameType[0], nameType[1]) ||
			ParseCreateCommand<CPtLight*>("ptlight", nameType[0], nameType[1]) ||
			ParseCreateCommand<CSpotLight*>("spotlight", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<int>>("intarray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<double>>("doublearray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<string>>("stringarray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<COLOR>>("colorarray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<CPt>>("ptarray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<UVC>>("uvcarray", nameType[0], nameType[1]) ||
			ParseCreateCommand<CArray<CUnitVector>>("unitvectorcarray", nameType[0], nameType[1]))
		{
			if (args.size() >= 2)
			{
				ValidateArgCount(args, 3, args[0]);
				if (args[1] == "=")
				{
					string root = nameType[0];
					args.erase(args.begin());
					args.erase(args.begin());

					vector<string> comps;
					comps.push_back("set");
					if (!ParsePrimitiveCommand(root, comps, args))
						ParseArrayCommand(root, comps, args);
				}
				else
					ThrowParseException("Invalid create command", args[0]);
			}
		}
		else
			ThrowParseException("Invalid create command", args[0]);

		// we consumed the command completely
		return true;
	}
	else
	{
		ValidateArgCount(args, 3, args[0]);
		if (args[1] != "=")
			ThrowParseException("Invalid create command", args[0]);
		m_returnVarName = args[0];
		args.erase(args.begin());
		args.erase(args.begin());
	}

	return false;
}

bool CParser2::ParsePrimitiveCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	if (comps.size() == 0)
		return false;

	if (ParsePrimitiveCommand<int>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<double>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<bool>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<string>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<COLOR>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<UVC>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<CPt>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<CUnitVector>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<CMatrix>(root, comps[0], args))
		return true;
	if (ParsePrimitiveCommand<CAnimRecord>(root, comps[0], args))
		return true;

	return false;
}

bool CParser2::ParseArrayCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	if (comps.size() == 0)
		return false;

	if (ParseArrayCommand<int>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<double>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<string>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<COLOR>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<CPt>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<CUnitVector>(root, comps[0], args))
		return true;
	if (ParseArrayCommand<UVC>(root, comps[0], args))
		return true;

	return false;
}

void CParser2::ParseScriptCommand(vector<string>& args)
{
	CParser2 parser(m_pmodel, m_panimmanager);
	parser.m_aspect = m_aspect;
	parser.m_aspectIsFixed = m_aspectIsFixed;
	parser.m_envpath = m_envpath;

	for (size_t i = 1; i < args.size(); i++)
	{
		if (!CopyEnvVarToNewParser<int>(args[i], parser) &&
			!CopyEnvVarToNewParser<double>(args[i], parser) &&
			!CopyEnvVarToNewParser<bool>(args[i], parser) &&
			!CopyEnvVarToNewParser<string>(args[i], parser) &&
			!CopyEnvVarToNewParser<COLOR>(args[i], parser) &&
			!CopyEnvVarToNewParser<UVC>(args[i], parser) &&
			!CopyEnvVarToNewParser<CPt>(args[i], parser) &&
			!CopyEnvVarToNewParser<CUnitVector>(args[i], parser) &&
			!CopyEnvVarToNewParser<CMatrix>(args[i], parser) &&
			!CopyEnvVarToNewParser<CGrp*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CSphere*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CPolygon*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CDirLight*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CPtLight*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CSpotLight*>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<int>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<double>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<string>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<COLOR>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<UVC>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<CPt>>(args[i], parser) &&
			!CopyEnvVarToNewParser<CArray<CUnitVector>>(args[i], parser))
			ThrowParseException("Argument not found", args[i]);
	}

	if (parser.ParseCommandScript(ParseObjectCached<string>(args[0])))
	{
		if (!m_returnVarName.empty() && !parser.m_scriptReturnVarName.empty())
		{
			CopyReturnValueFromOtherParser<int>(parser);
			CopyReturnValueFromOtherParser<double>(parser);
			CopyReturnValueFromOtherParser<bool>(parser);
			CopyReturnValueFromOtherParser<string>(parser);
			CopyReturnValueFromOtherParser<COLOR>(parser);
			CopyReturnValueFromOtherParser<UVC>(parser);
			CopyReturnValueFromOtherParser<CPt>(parser);
			CopyReturnValueFromOtherParser<CUnitVector>(parser);
			CopyReturnValueFromOtherParser<CMatrix>(parser);
			CopyReturnValueFromOtherParser<CGrp*>(parser);
			CopyReturnValueFromOtherParser<CSphere*>(parser);
			CopyReturnValueFromOtherParser<CPolygon*>(parser);
			CopyReturnValueFromOtherParser<CDirLight*>(parser);
			CopyReturnValueFromOtherParser<CPtLight*>(parser);
			CopyReturnValueFromOtherParser<CSpotLight*>(parser);
			CopyReturnValueFromOtherParser<CArray<int>>(parser);
			CopyReturnValueFromOtherParser<CArray<double>>(parser);
			CopyReturnValueFromOtherParser<CArray<string>>(parser);
			CopyReturnValueFromOtherParser<CArray<COLOR>>(parser);
			CopyReturnValueFromOtherParser<CArray<UVC>>(parser);
			CopyReturnValueFromOtherParser<CArray<CPt>>(parser);
			CopyReturnValueFromOtherParser<CArray<CUnitVector>>(parser);
		}
	}
	else
		ThrowParseException(parser.m_serr, "Script Error");
}

void CParser2::ParseMatrixCommand(CMatrix& matrix, const string& command, vector<string>& args)
{
	if (command == "identity")
	{
		matrix.SetIdentity();
	}
	else if (command == "set")
	{
		ValidateArgCount(args, 1, command);
		CMatrix& other = GetFromVarMap<CMatrix>(args[0]);
		matrix = other;
	}
	else if (command == "rotatex")
	{
		ValidateArgCount(args, 1, command);
		matrix.RotateX(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "rotatey")
	{
		ValidateArgCount(args, 1, command);
		matrix.RotateY(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "rotatez")
	{
		ValidateArgCount(args, 1, command);
		matrix.RotateZ(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "translate")
	{
		ValidateArgCount(args, 1, command);

		CPt vector = ParseObjectCached<CPt>(args[0]);
		matrix.Translate(vector.x, vector.y, vector.z);
	}
	else if (command == "scale")
	{
		ValidateArgCount(args, 1, command);

		CPt vector = ParseObjectCached<CPt>(args[0]);
		matrix.Scale(vector.x, vector.y, vector.z);
	}
	else
		ThrowParseException("Invalid matrix command", command);
}

void CParser2::ParseCameraCommand(vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
		ThrowParseException("Command expects at least one sub-component", "camera");
	string first = comps[0];
	comps.erase(comps.begin());

	if (first == "matrix")
	{
		ParseMatrixCommand(m_pmodel->GetCamera().GetTM(), comps[0], args);
	}
	else if (first == "aspect")
	{
		if (!m_aspectIsFixed)
		{
			ValidateArgCount(args, 1, first);
			m_aspect = ParseObjectCached<double>(args[0]);
		}
	}
	else if (first == "viewport")
	{
		ValidateArgCount(args, 2, first);

		double minvp = ParseObjectInvertible<double>(args[0]);
		double dist = ParseObjectInvertible<double>(args[1]);
		double aspect = (args.size() > 2 && m_aspect == 0 ? ParseObjectCached<double>(args[2]) : m_aspect);

		double ulen = (aspect > 1 ? minvp * aspect : minvp);
		double vlen = (aspect < 1 ? minvp / aspect : minvp);
		m_pmodel->GetCamera().SetViewport(ulen, vlen, dist);
	}
	else
		ThrowParseException("Invalid camera command", first);
}

void CParser2::ParseBgCommand(vector<string>& comps, vector<string>& args)
{
	string first = comps[0];
	comps.erase(comps.begin());

	if (first == "colors")
	{
		ValidateArgCount(args, 3, first);

		COLOR n = ParseObjectCached<COLOR>(args[0]);
		COLOR e = ParseObjectCached<COLOR>(args[1]);
		COLOR s = ParseObjectCached<COLOR>(args[2]);
		m_pmodel->GetBackgroundHandler().SetBackgroundColors(n, e, s);
	}
	else if (first == "image")
	{
		ValidateArgCount(args, 2, first);

		string path = ParseObjectCached<string>(args[0]);
		ImageResize isize = ParseObject<ImageResize>(args[1]);
		m_pmodel->GetBackgroundHandler().SetBackgroundImage(path, isize);
	}
	else
		ThrowParseException("Invalid background command", first);
}

void CParser2::ParseImageMapCommand(vector<string>& comps, vector<string>& args)
{
	string first = comps[0];
	comps.erase(comps.begin());

	if (first == "load")
	{
		ValidateArgCount(args, 1, first);
		string path = ParseObjectCached<string>(args[0]);
		ImageFormat fmt = (args.size() > 1 ? ParseObject<ImageFormat>(args[1]) : ImageFormat::None);
		string title = (args.size() > 2 ? ParseObjectCached<string>(args[2]) : "");

		title = m_pmodel->GetImageMap().LoadImageFile(path, fmt, title);
		if (title.empty())
			ThrowParseException("Bad path", path);
		AssignReturnValue<>(title);
	}
	else if (first == "deleteall")
	{
		m_pmodel->GetImageMap().DeleteAll();
	}
	else
	{
		first = ParseObjectCached<string>(first);
		if (!ParseImagesCommand(first, comps, args))
			ThrowParseException("Invalid image map command", first);
	}
}

bool CParser2::ParseImagesCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	CImageBuffer* pimage = m_pmodel->GetImageMap().GetImage(root);
	if (pimage != NULL)
	{
		string command = comps[0];

		if (command == "fillalphafromcolors")
		{
			AssignReturnValue<>(pimage->FillAlphaFromColors());
		}
		else if (command == "fillalphafromimage")
		{
			ValidateArgCount(args, 1, command);
			string other = ParseObjectCached<string>(args[0]);
			CImageBuffer* pother = m_pmodel->GetImageMap().GetImage(other);
			if (pother != NULL)
				AssignReturnValue<>(pimage->FillAlphaFromImage(*pother));
			else
				ThrowParseException("Unknown image", other);
		}
		else if (command == "fillalphafromtransparentcolor")
		{
			ValidateArgCount(args, 7, command);
			int r = ParseObjectCached<int>(args[0]);
			int g = ParseObjectCached<int>(args[1]);
			int b = ParseObjectCached<int>(args[2]);
			int dr = ParseObjectCached<int>(args[3]);
			int dg = ParseObjectCached<int>(args[4]);
			int db = ParseObjectCached<int>(args[5]);
			int a = ParseObjectCached<int>(args[6]);
			AssignReturnValue<>(pimage->FillAlphaFromTransparentColor(r, g, b, dr, dg, db, a));
		}
		else if (command == "normalizealpha")
		{
			pimage->NormalizeAlpha();
		}
		else
			ThrowParseException("Invalid image command", command);

		return true;
	}

	return false;
}

void CParser2::ParseAnimRecordValues(CAnimRecord& record, const string& beginStr, const string& endStr)
{
	switch (record.GetAnimType())
	{
	case AnimType::RotateX:
	case AnimType::RotateY:
	case AnimType::RotateZ:
	case AnimType::CameraDist:
	case AnimType::CameraULen:
	case AnimType::CameraVLen:
	case AnimType::LightHLit:
	case AnimType::LightSScale:
	case AnimType::LightFullDistance:
	case AnimType::LightDropDistance:
	case AnimType::ModelFade:
	case AnimType::ModelFog:
	case AnimType::ModelFogNear:
	case AnimType::ModelFogFar:
	{
		double beginValue = ParseObjectCached<double>(beginStr);
		double endValue = (!endStr.empty() ? ParseObjectCached<double>(endStr) : beginValue);
		record.GetValues<double>().SetValues(beginValue, endValue);
	}
	break;

	case AnimType::ColorDiffuse:
	case AnimType::ColorSpecular:
	case AnimType::ColorReflection:
	case AnimType::ColorRefraction:
	case AnimType::ColorGlow:
	case AnimType::BgColorNorth:
	case AnimType::BgColorEquator:
	case AnimType::BgColorSouth:
	case AnimType::LightColor:
	case AnimType::ModelAmbient:
	{
		COLOR beginValue = ParseObjectCached<COLOR>(beginStr);
		COLOR endValue = (!endStr.empty() ? ParseObjectCached<COLOR>(endStr) : beginValue);
		record.GetValues<COLOR>().SetValues(beginValue, endValue);
	}
	break;

	case AnimType::Translate:
	case AnimType::Scale:
	{
		CPt beginValue = ParseObjectCached<CPt>(beginStr);
		CPt endValue = (!endStr.empty() ? ParseObjectCached<CPt>(endStr) : beginValue);
		record.GetValues<CPt>().SetValues(beginValue, endValue);
	}
	break;

	case AnimType::None:
		break;

	default:
		throw parse_exception("Unsupported animation type");
	}
}

void CParser2::ParseAnimationCommand(vector<string>& comps, vector<string>& args)
{
	string first = comps[0];
	comps.erase(comps.begin());

	if (first == "add")
	{
		ValidateArgCount(args, 2, first);
		string objName = ParseObjectCached<string>(args[0]);
		try
		{
			objName = GenerateObjectNamespace(&m_pmodel->GetSuperGroup(), objName);
		}
		catch (const parse_exception&)
		{
			// this is ok, might be an animation constant like model, camera, etc.
		}
		if (objName.empty())
			ThrowParseException("Cannot locate group", args[0]);
		string varOrType = ParseObjectCached<string>(args[1]);

		CAnimRecord record;
		if (InVarMap<CAnimRecord>(varOrType))
		{
			record = GetFromVarMap<CAnimRecord>(varOrType);
		}
		else
		{
			record.SetAnimType(ParseEnum<AnimType>(varOrType));
		}

		if (args.size() < 4)
		{
			m_panimmanager->AddRecord(objName, record);
		}
		else
		{
			double beginTime = ParseObjectCached<double>(args[2]);
			double endTime = ParseObjectCached<double>(args[3]);
			if (args.size() > 4)
				ParseAnimRecordValues(record, ParseObjectCached<string>(args[4]), args.size() > 5 ? ParseObjectCached<string>(args[5]) : "");
			m_panimmanager->AddRecord(objName, beginTime, endTime, record);
		}
		AssignReturnValue<CAnimRecord>(record);
	}
	else if (first == "load")
	{
		ValidateArgCount(args, 2, first);
		string objName = ParseObjectCached<string>(args[0]);
		try
		{
			objName = GenerateObjectNamespace(&m_pmodel->GetSuperGroup(), objName);
		}
		catch (const parse_exception&)
		{
			// this is ok, might be an animation constant like model, camera, etc.
		}
		if (objName.empty())
			ThrowParseException("Cannot locate object", args[0]);
		string path = ParseObjectCached<string>(args[1]);

		CAnimRecord record = JsonReader::LoadFromJsonFile<CAnimRecord>(path, m_envpath);
		if (args.size() < 4)
		{
			m_panimmanager->AddRecord(objName, record);
		}
		else
		{
			double beginTime = ParseObjectCached<double>(args[2]);
			double endTime = ParseObjectCached<double>(args[3]);
			if (args.size() == 4)
			{
				m_panimmanager->AddRecord(objName, beginTime, endTime, record);
			}
			else
			{
				bool applyToStart = (args.size() > 4 ? ParseObjectCached<bool>(args[4]) : false);
				bool reverseAnim = (args.size() > 5 ? ParseObjectCached<bool>(args[5]) : false);
				m_panimmanager->AddRecord(objName, beginTime, endTime, applyToStart, reverseAnim, record);
			}
		}
		AssignReturnValue<CAnimRecord>(record);
	}
	else if (first == "newrecord")
	{
		CAnimRecord record;
		if (args.size() > 0)
		{
			string pathOrType = ParseObjectCached<string>(args[0]);
			if (pathOrType.find(".json") != string::npos)
				record = JsonReader::LoadFromJsonFile<CAnimRecord>(pathOrType, m_envpath);
			else
				record.SetAnimType(ParseEnum<AnimType>(pathOrType));
		}
		if (args.size() > 1)
			record.SetTimes(ParseObjectCached<double>(args[1]), args.size() > 2 ? ParseObjectCached<double>(args[2]) : 1);
		AssignReturnValue<CAnimRecord>(record);
	}
}

void CParser2::ParseBaseObjectCommand(CBaseObj* pobj, const string& command, vector<string>& comps, vector<string>& args)
{
	if (command == "matrix")
	{
		comps.erase(comps.begin());
		ParseMatrixCommand(pobj->GetTM(), comps[0], args);
	}
	else if (command == "meta")
	{
		ValidateArgCount(args, 1, command);
		string key = ParseObjectCached<string>(args[0]);
		if (args.size() == 1)
		{
			char buf[_MAX_PATH];
			if (pobj->GetMetaData(key.c_str(), buf, _MAX_PATH))
				AssignReturnValue<>(buf);
			else
				ThrowParseException("Meta data value does not exist", key);
		}
		else
		{
			string value = ParseObjectCached<string>(args[1]);
			pobj->AddMetaData(key.c_str(), value.c_str());
		}
	}
	else if (command == "load")
	{
		ValidateArgCount(args, 1, command);
		string path = ParseObjectCached<string>(args[0]);
		CBinFile binFile(path.c_str());
		if (!binFile.OpenFile(false, MigType::Object))
			ThrowParseException("Unable to open file", path);
		pobj->Load(binFile);
		binFile.CloseFile();
	}
	else if (command == "save")
	{
		ValidateArgCount(args, 1, command);
		string path = ParseObjectCached<string>(args[0]);
		CBinFile binFile(path.c_str());
		if (!binFile.OpenFile(true, MigType::Object))
			ThrowParseException("Unable to open file", path);
		pobj->Save(binFile);
		binFile.CloseFile();
	}
	else if (command == "name")
	{
		if (args.size() == 0)
			AssignReturnValue<>(pobj->GetName());
		else
			pobj->SetName(ParseObjectCached<string>(args[0]));
	}
	else
		ThrowParseException("Invalid base object command", command);
}

static void DoFitCommand(CBaseObj* pobj, CBoundBox bbox, double fitX, double fitY, double fitZ)
{
	CPt minpt = bbox.GetMinPt();
	CPt maxpt = bbox.GetMaxPt();

	double scale = -1;
	if (fitX > 0)
	{
		double lenX = maxpt.x - minpt.x;
		scale = (scale == -1 ? fitX / lenX : min(scale, fitX / lenX));
	}
	if (fitY > 0)
	{
		double lenY = maxpt.y - minpt.y;
		scale = (scale == -1 ? fitY / lenY : min(scale, fitY / lenY));
	}
	if (fitZ > 0)
	{
		double lenZ = maxpt.z - minpt.z;
		scale = (scale == -1 ? fitZ / lenZ : min(scale, fitZ / lenZ));
	}

	if (scale != -1)
		pobj->GetTM().Scale(scale, scale, scale);
}

void CParser2::ParseGroupCommand(CGrp* pgrp, vector<string>& comps, vector<string>& args)
{
	string command = comps[0];

	if (command == "newgroup")
	{
		CGrp* pnew = pgrp->CreateSubGroup();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newpolygon")
	{
		CPolygon* pnew = pgrp->CreatePolygon();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newsphere")
	{
		CSphere* pnew = pgrp->CreateSphere();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newdirlight")
	{
		CDirLight* pnew = pgrp->CreateDirectionalLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newptlight")
	{
		CPtLight* pnew = pgrp->CreatePointLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newspotlight")
	{
		CSpotLight* pnew = pgrp->CreateSpotLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "center")
	{
		CBoundBox bbox;
		if (pgrp->ComputeBoundBox(bbox))
		{
			CPt center = bbox.GetCenter();
			pgrp->GetTM().Translate(-center.x, -center.y, -center.z);
		}
	}
	else if (command == "fit")
	{
		double fitX = (args.size() > 0 ? ParseObjectCached<double>(args[0]) : -1);
		double fitY = (args.size() > 1 ? ParseObjectCached<double>(args[1]) : -1);
		double fitZ = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : -1);

		CBoundBox bbox;
		if (pgrp->ComputeBoundBox(bbox))
			DoFitCommand(pgrp, bbox, fitX, fitY, fitZ);
	}
	else
		ParseBaseObjectCommand(pgrp, command, comps, args);
}

void CParser2::ParseLightCommand(CLight* plight, vector<string>& comps, vector<string>& args)
{
	bool found = true;
	string command = comps[0];

	if (plight->GetType() == ObjType::DirLight)
	{
		CDirLight* pdirlight = (CDirLight*)plight;
		if (command == "dir")
		{
			ValidateArgCount(args, 1, command);
			pdirlight->SetDirection(ParseObjectCached<CUnitVector>(args[0]));
		}
		else
			found = false;
	}
	else if (plight->GetType() == ObjType::PtLight)
	{
		CPtLight* pptlight = (CPtLight*)plight;
		if (command == "origin")
		{
			ValidateArgCount(args, 1, command);
			pptlight->SetOrigin(ParseObjectCached<CPt>(args[0]));
		}
		else if (command == "dropoff")
		{
			ValidateArgCount(args, 2, command);
			pptlight->SetDropoff(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
		}
		else
			found = false;
	}
	else if (plight->GetType() == ObjType::SpotLight)
	{
		CSpotLight* pspot = (CSpotLight*)plight;
		if (command == "origin")
		{
			ValidateArgCount(args, 1, command);
			pspot->SetOrigin(ParseObjectCached<CPt>(args[0]));
		}
		else if (command == "dir")
		{
			ValidateArgCount(args, 1, command);
			pspot->SetDirection(ParseObjectCached<CUnitVector>(args[0]));
		}
		else if (command == "concentration")
		{
			ValidateArgCount(args, 1, command);
			pspot->SetConcentration(ParseObjectCached<double>(args[0]));
		}
		else if (command == "dropoff")
		{
			ValidateArgCount(args, 2, command);
			pspot->SetDropoff(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
		}
		else
			found = false;
	}
	else
		ThrowParseException("Unknown light type", command);

	if (!found)
	{
		if (command == "color")
		{
			ValidateArgCount(args, 1, command);
			plight->SetColor(ParseObjectCached<COLOR>(args[0]));
		}
		else if (command == "highlight")
		{
			ValidateArgCount(args, 1, command);
			plight->SetHighlight(ParseObjectCached<double>(args[0]));
		}
		else if (command == "shadow")
		{
			ValidateArgCount(args, 1, command);
			bool shadow = ParseObjectCached<bool>(args[0]);
			bool soft = (args.size() > 1 ? ParseObjectCached<bool>(args[1]) : false);
			double sscale = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : 1);
			plight->SetShadowCaster(shadow, soft, sscale);
		}
		else if (command == "matrix")
		{
			comps.erase(comps.begin());
			ParseMatrixCommand(plight->GetTM(), comps[0], args);
		}
		else
			ParseBaseObjectCommand(plight, command, comps, args);
	}
}

void CParser2::ParseObjectCommand(CObj* pobj, const string& command, vector<string>& comps, vector<string>& args)
{
	if (command == "diffuse")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetDiffuse(ParseObjectCached<COLOR>(args[0]));
	}
	else if (command == "specular")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetSpecular(ParseObjectCached<COLOR>(args[0]));
	}
	else if (command == "reflection")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetReflection(ParseObjectCached<COLOR>(args[0]));
	}
	else if (command == "refraction")
	{
		ValidateArgCount(args, 1, command);
		double index = (args.size() > 1 ? ParseObjectCached<double>(args[1]) : 1);
		double near = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : -1);
		double far = (args.size() > 3 ? ParseObjectCached<double>(args[3]) : -1);
		pobj->SetRefraction(ParseObjectCached<COLOR>(args[0]), index, near, far);
	}
	else if (command == "glow")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetGlow(ParseObjectCached<COLOR>(args[0]));
	}
	else if (command == "flags")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetObjectFlags(ParseObjectFlags(args[0]));
	}
	else if (command == "bbox")
	{
		ValidateArgCount(args, 1, command);
		pobj->SetBoundBox(ParseObjectCached<bool>(args[0]));
	}
	else if (command == "matrix")
	{
		comps.erase(comps.begin());
		ParseMatrixCommand(pobj->GetTM(), comps[0], args);
	}
	else if (command == "center")
	{
		CBoundBox bbox;
		pobj->ComputeBoundBox(bbox, CMatrix());
		CPt center = bbox.GetCenter();
		pobj->GetTM().Translate(-center.x, -center.y, -center.z);
	}
	else if (command == "fit")
	{
		double fitX = (args.size() > 0 ? ParseObjectCached<double>(args[0]) : -1);
		double fitY = (args.size() > 1 ? ParseObjectCached<double>(args[1]) : -1);
		double fitZ = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : -1);

		CBoundBox bbox;
		pobj->ComputeBoundBox(bbox, CMatrix());
		DoFitCommand(pobj, bbox, fitX, fitY, fitZ);
	}
	else
		ParseBaseObjectCommand(pobj, command, comps, args);
}

void CParser2::ParseSphereCommand(CSphere* psphere, vector<string>& comps, vector<string>& args)
{
	string command = comps[0];

	if (command == "origin")
	{
		ValidateArgCount(args, 1, command);
		psphere->SetOrigin(ParseObjectCached<CPt>(args[0]));
	}
	else if (command == "radius")
	{
		ValidateArgCount(args, 1, command);
		psphere->SetRadius(ParseObjectCached<double>(args[0]));
	}
	else if (command == "addcolormap")
	{
		ValidateArgCount(args, 3, command);
		TextureMapType tmap = ParseObject<TextureMapType>(args[0]);
		dword flags = ParseTexturingFlags(args[1]);
		string title = ParseObjectCached<string>(args[2]);
		TextureMapOp op = (args.size() > 3 ? ParseObject<TextureMapOp>(args[3]) : TextureMapOp::Multiply);
		UVC uvMin = (args.size() > 4 ? ParseObjectCached<UVC>(args[4]) : UVMIN);
		UVC uvMax = (args.size() > 5 ? ParseObjectCached<UVC>(args[5]) : UVMAX);
		AssignReturnValue<>(psphere->AddColorMap(tmap, op, flags, title, uvMin, uvMax));
	}
	else if (command == "addtransparencymap")
	{
		ValidateArgCount(args, 2, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(psphere->AddTransparencyMap(flags, title, uvMin, uvMax));
	}
	else if (command == "addbumpmap")
	{
		ValidateArgCount(args, 4, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		double scale = ParseObjectCached<double>(args[2]);
		int btol = ParseObjectCached<int>(args[3]);
		UVC uvMin = (args.size() > 4 ? ParseObjectCached<UVC>(args[4]) : UVMIN);
		UVC uvMax = (args.size() > 5 ? ParseObjectCached<UVC>(args[5]) : UVMAX);
		AssignReturnValue<>(psphere->AddBumpMap(flags, title, scale, btol, uvMin, uvMax));
	}
	else if (command == "addreflectionmap")
	{
		ValidateArgCount(args, 2, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(psphere->AddReflectionMap(flags, title, uvMin, uvMax));
	}
	else if (command == "duplicate")
	{
		ValidateArgCount(args, 1, command);
		CSphere* pnew = ParseObjectCached<CSphere*>(args[0]);
		if (pnew == NULL)
			ThrowParseException("New sphere does not exist", command);
		*pnew = *psphere;
	}
	else
		ParseObjectCommand(psphere, command, comps, args);
}

void CParser2::ParsePolygonCommand(CPolygon* ppoly, vector<string>& comps, vector<string>& args)
{
	string command = comps[0];

	if (command == "loadlattice")
	{
		ValidateArgCount(args, 1, command);

		CArray<CPt> lattice = ParseArray<CPt>(args[0]);
		if (lattice.GetSize() > 0)
			AssignReturnValue<>(ppoly->LoadLattice(lattice.GetSize(), lattice.ToArray()));
		else
			ThrowParseException("Invalid lattice command", args[0]);
	}
	else if (command == "loadlatticept")
	{
		ValidateArgCount(args, 1, command);

		CPt pt = ParseObjectCached<CPt>(args[0]);
		AssignReturnValue<>(ppoly->LoadLatticePt(pt));
	}
	else if (command == "addpolycurve")
	{
		ValidateArgCount(args, 4, command);
		CUnitVector norm = ParseObjectCached<CUnitVector>(args[0]);
		dword flags = ParsePolyCurveFlags(args[1]);
		CArray<int> inds = ParseArray<int>(args[2]);
		CArray<CUnitVector> norms = ParseArray<CUnitVector>(args[3]);

		if (inds.GetSize() && norms.GetSize() && inds.GetSize() != norms.GetSize())
			ThrowParseException("Invalid poly curve command", command);
		AssignReturnValue<>(ppoly->AddPolyCurve(norm, flags, inds.ToArray(), norms.ToArray(), max(inds.GetSize(), norms.GetSize())));
	}
	else if (command == "addpolycurveindex")
	{
		ValidateArgCount(args, 1, command);
		int index = ParseObjectCached<int>(args[0]);
		AssignReturnValue<>(ppoly->AddPolyCurveIndex(index));
	}
	else if (command == "addpolycurvenormal")
	{
		ValidateArgCount(args, 1, command);
		CUnitVector norm = ParseObjectCached<CUnitVector>(args[0]);
		AssignReturnValue<>(ppoly->AddPolyCurveNormal(norm));
	}
	else if (command == "addpolycurvemapcoord")
	{
		ValidateArgCount(args, 3, command);
		TextureMapType tmap = ParseObject<TextureMapType>(args[0]);
		int index = ParseObjectCached<int>(args[1]);
		UVC uvc = ParseObjectCached<UVC>(args[2]);
		AssignReturnValue<>(ppoly->AddPolyCurveMapCoord(tmap, index, uvc));
	}
	else if (command == "setclockedness")
	{
		ValidateArgCount(args, 2, command);
		AssignReturnValue<>(ppoly->SetClockedness(ParseObjectCached<bool>(args[0]), ParseObjectCached<bool>(args[1])));
	}
	else if (command == "extrude")
	{
		ValidateArgCount(args, 2, command);
		if (args.size() > 2)
		{
			CArray<double> ang = ParseArray<double>(args[0]);
			CArray<double> len = ParseArray<double>(args[1]);
			bool smooth = ParseObjectCached<bool>(args[2]);
			if (ang.GetSize() == 0 || len.GetSize() == 0 || ang.GetSize() != len.GetSize())
				ThrowParseException("Invalid extrude command", command);
			AssignReturnValue<>(ppoly->Extrude(ang.ToArray(), len.ToArray(), ang.GetSize(), smooth));
		}
		else
		{
			double length = ParseObjectCached<double>(args[0]);
			bool smooth = ParseObjectCached<bool>(args[1]);
			AssignReturnValue<>(ppoly->Extrude(length, smooth));
		}
	}
	else if (command == "addcolormap")
	{
		ValidateArgCount(args, 3, command);
		TextureMapType tmap = ParseObject<TextureMapType>(args[0]);
		dword flags = ParseTexturingFlags(args[1]);
		string title = ParseObjectCached<string>(args[2]);
		CArray<UVC> uvc = (args.size() > 3 ? ParseArray<UVC>(args[3]) : CArray<UVC>());
		TextureMapOp op = (args.size() > 4 ? ParseObject<TextureMapOp>(args[4]) : TextureMapOp::Multiply);
		UVC uvMin = (args.size() > 5 ? ParseObjectCached<UVC>(args[5]) : UVMIN);
		UVC uvMax = (args.size() > 6 ? ParseObjectCached<UVC>(args[6]) : UVMAX);
		AssignReturnValue<>(ppoly->AddColorMap(tmap, op, flags, title, uvc.ToArray(), uvc.GetSize(), uvMin, uvMax));
	}
	else if (command == "addtransparencymap")
	{
		ValidateArgCount(args, 2, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		CArray<UVC> uvc = (args.size() > 2 ? ParseArray<UVC>(args[2]) : CArray<UVC>());
		UVC uvMin = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMIN);
		UVC uvMax = (args.size() > 4 ? ParseObjectCached<UVC>(args[4]) : UVMAX);
		AssignReturnValue<>(ppoly->AddTransparencyMap(flags, title, uvc.ToArray(), uvc.GetSize(), uvMin, uvMax));
	}
	else if (command == "addbumpmap")
	{
		ValidateArgCount(args, 2, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		double scale = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : 1);
		int btol = (args.size() > 3 ? ParseObjectCached<int>(args[3]) : 0);
		CArray<UVC> uvc = (args.size() > 4 ? ParseArray<UVC>(args[4]) : CArray<UVC>());
		UVC uvMin = (args.size() > 5 ? ParseObjectCached<UVC>(args[5]) : UVMIN);
		UVC uvMax = (args.size() > 6 ? ParseObjectCached<UVC>(args[6]) : UVMAX);
		AssignReturnValue<>(ppoly->AddBumpMap(flags, title, scale, btol, uvc.ToArray(), uvc.GetSize(), uvMin, uvMax));
	}
	else if (command == "addreflectionmap")
	{
		ValidateArgCount(args, 2, command);
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(ppoly->AddReflectionMap(flags, title, uvMin, uvMax));
	}
	else if (command == "applymapping")
	{
		ValidateArgCount(args, 4, command);
		TextureMapWrapType wtype = ParseObject<TextureMapWrapType>(args[0]);
		TextureMapType tmap = ParseObject<TextureMapType>(args[1]);
		int mapping = ParseObjectCached<int>(args[2]);
		bool indc = ParseObjectCached<bool>(args[3]);
		unique_ptr<CPt> pcenter = (args.size() > 4 ? ParseObjectNullable<CPt>(args[4]) : NULL);
		unique_ptr<CUnitVector> paxis = (args.size() > 5 ? ParseObjectNullable<CUnitVector>(args[5]) : NULL);
		unique_ptr<UVC> uvmin = (args.size() > 6 ? ParseObjectNullable<UVC>(args[6]) : NULL);
		unique_ptr<UVC> uvmax = (args.size() > 7 ? ParseObjectNullable<UVC>(args[7]) : NULL);
		AssignReturnValue<>(ppoly->ApplyMapping(wtype, tmap, mapping, indc, pcenter.get(), paxis.get(), uvmin.get(), uvmax.get()));
	}
	else if (command == "duplicate")
	{
		ValidateArgCount(args, 1, command);
		CPolygon* pnew = ParseObjectCached<CPolygon*>(args[0]);
		if (pnew == NULL)
			ThrowParseException("New polygon does not exist", command);
		*pnew = *ppoly;
	}
	else if (command == "dupfaceplate")
	{
		ValidateArgCount(args, 2, command);
		CPolygon* pnew = ParseObjectCached<CPolygon*>(args[0]);
		bool hideOriginalFacePlate = ParseObjectCached<bool>(args[1]);
		if (pnew == NULL)
			ThrowParseException("New polygon does not exist", command);
		ppoly->DupExtrudedFacePlate(pnew, hideOriginalFacePlate);
	}
	else if (command == "complete")
	{
		AssignReturnValue<>(ppoly->LoadComplete());
	}
	else
		ParseObjectCommand(ppoly, command, comps, args);
}

static string GetTitleFromPath(const string& path)
{
	size_t slash = path.rfind('\\');
	if (slash == -1)
		return path;
	return path.substr(slash + 1);
}

static string FormatError(const string& script, int line, const string& error)
{
	string prefix;
	if (!script.empty() && line > 0)
		prefix = "[" + GetTitleFromPath(script) + "::" + to_string(line) + "] ";
	return prefix + error;
}

static bool LoadTextString(CPackage& package, CModel& model, CGrp* pgrp, const string& text, const string& suffix, const string& script, const string& envpath)
{
	int incx = 0, incy = 0;
	for (size_t n = 0; n < text.length(); n++)
	{
		char item[24] = {};
		//memset(item, 0, sizeof(item));
		item[0] = text[n];
		item[1] = '-';
		if (!suffix.empty())
			strcat(item, suffix.c_str());

		//if (package.GetObjectType(item) == BlockType::Polygon)
		if (package.GetObjectType(item) == BlockType::Group)
		{
			//CPolygon* pobj = psub->CreatePolygon();
			CGrp* pobj = pgrp->CreateSubGroup();
			package.LoadObject(item, pobj);

			// helps modulate the uv coordinates of the texture mappings
			int xtxtdiv = 2, ytxtdiv = 2;
			UVC uvmin, uvmax;
			uvmin.u = (double)(n % xtxtdiv) / xtxtdiv;
			uvmin.v = (double)((n / ytxtdiv) % ytxtdiv) / ytxtdiv;
			uvmax.u = uvmin.u + (1.0 / xtxtdiv);
			uvmax.v = uvmin.v + (1.0 / ytxtdiv);

			if (!script.empty())
			{
				CParser2 parser(&model);
				parser.SetEnvPath(envpath);
				parser.SetEnvVarModelRef<CGrp>("grp", pobj);
				parser.SetEnvVarModelRef<CPolygon>("poly", (CPolygon*)pobj->GetObjects()[0].get());
				parser.SetEnvVar<UVC>("uvmin", uvmin);
				parser.SetEnvVar<UVC>("uvmax", uvmax);
				parser.ParseCommandScript(script.c_str());
			}

			pobj->GetTM().Translate(incx, incy, 0);

			char buf[16];
			pobj->GetMetaData("cellinc", buf, sizeof(buf));
			incx += atoi(strtok(buf, ","));
			incy += atoi(strtok(NULL, ","));
		}
	}

	return true;
}

#define LOAD_FROM_PACKAGE(BLOCK_TYPE_REF, OBJ_TYPE, CREATE_FUNC) \
if (bt == BLOCK_TYPE_REF) { \
	OBJ_TYPE* pobj = pgrp->CREATE_FUNC(); \
	package.LoadObject(name.c_str(), pobj); \
	AssignReturnValue<>(pobj); \
}

void CParser2::InitCommandHandlers(void)
{
	if (!m_cmdHandlers.empty())
		return;

	m_cmdHandlers.emplace("sampling", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			m_pmodel->SetSampling(ParseObject<SuperSample>(args[0]));
		}
	));

	m_cmdHandlers.emplace("renderflags", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			m_pmodel->SetRenderQuality(ParseRenderFlags(args[0]));
		}
	));

	m_cmdHandlers.emplace("ambient", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			m_pmodel->SetAmbientLight(ParseObjectCached<COLOR>(args[0]));
		}
	));

	m_cmdHandlers.emplace("fade", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			m_pmodel->SetFade(ParseObjectCached<COLOR>(args[0]));
		}
	));

	m_cmdHandlers.emplace("fog", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 3, cmd);
			m_pmodel->SetFog(
				ParseObjectCached<COLOR>(args[0]),
				ParseObjectCached<double>(args[1]),
				ParseObjectCached<double>(args[2]));
		}
	));

	m_cmdHandlers.emplace("load", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			string path = ParseObjectCached<string>(args[0]);
			CBinFile binFile(path.c_str());
			if (!binFile.OpenFile(false, MigType::Model))
				ThrowParseException("Unable to open file", path);
			m_pmodel->Load(binFile);
			binFile.CloseFile();
		}
	));

	m_cmdHandlers.emplace("save", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			string path = ParseObjectCached<string>(args[0]);
			CBinFile binFile(path.c_str());
			if (!binFile.OpenFile(true, MigType::Model))
				ThrowParseException("Unable to open file", path);
			m_pmodel->Save(binFile);
			binFile.CloseFile();
		}
	));

	m_cmdHandlers.emplace("loadpackage", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 2, cmd);
			string path = ParseObjectCached<string>(args[0]);
			string name = ParseObjectCached<string>(args[1]);
			CGrp* pgrp = (args.size() > 2 ? ParseObjectCached<CGrp*>(args[2]) : &m_pmodel->GetSuperGroup());

			CBinFile binFile(path.c_str());
			CPackage package;
			if (package.OpenPackage(&binFile) == 0)
				ThrowParseException("Unable to open package", path);
			BlockType bt = package.GetObjectType(name.c_str());
			LOAD_FROM_PACKAGE(BlockType::Polygon, CPolygon, CreatePolygon)
				LOAD_FROM_PACKAGE(BlockType::Group, CGrp, CreateSubGroup)
				LOAD_FROM_PACKAGE(BlockType::Sphere, CSphere, CreateSphere)
				LOAD_FROM_PACKAGE(BlockType::DirLight, CDirLight, CreateDirectionalLight)
				LOAD_FROM_PACKAGE(BlockType::PtLight, CPtLight, CreatePointLight)
				LOAD_FROM_PACKAGE(BlockType::SpotLight, CSpotLight, CreateSpotLight)
		}
	));

	m_cmdHandlers.emplace("loadtext", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 3, cmd);
			string pkg = ParseObjectCached<string>(args[0]);
			string text = ParseObjectCached<string>(args[1]);
			CGrp* pgrp = ParseObjectCached<CGrp*>(args[2]);
			string script = (args.size() > 3 ? ParseObjectCached<string>(args[3]) : "");
			string suffix = (args.size() > 4 ? ParseObjectCached<string>(args[4]) : "");

			CBinFile binFile(pkg.c_str());
			CPackage package;
			if (package.OpenPackage(&binFile) == 0)
				ThrowParseException("Unable to open package", pkg);
			AssignReturnValue<>(LoadTextString(package, *m_pmodel, pgrp, text, suffix, script, m_envpath));
		}
	));

	m_cmdHandlers.emplace("script", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			ParseScriptCommand(args);
		}
	));

	m_cmdHandlers.emplace("return", CParserCommand(
		[this](const std::string& cmd, std::vector<std::string>& args)
		{
			ValidateArgCount(args, 1, cmd);
			m_scriptReturnVarName = args[0];
		}
	));

}

bool CParser2::ParseCommandStringInner(const string& line)
{
	vector<string> args;
	if (SplitRootString(line, args) == 0)
		return true;

	string root = args[0];
	args.erase(args.begin());

	try
	{
		// create variable command
		bool bContinue = true;
		if (root == "var")
		{
			ValidateArgCount(args, 1, root);
			if (!ParseCreateCommand(args))
			{
				// continue processing, so update the root
				root = args[0];
				args.erase(args.begin());
			}
			else
				bContinue = false;
		}

		if (bContinue)
		{
			// global model commands
			if (m_cmdHandlers.find(root) != m_cmdHandlers.end())
			{
				m_cmdHandlers[root].Execute(root, args);
			}
			else
			{
				// split the root into it's nexted components (ie. "object.command")
				vector<string> comp;
				if (SplitNestedComponents(root, comp) > 0)
				{
					string rootComponent = comp[0];
					comp.erase(comp.begin());

					if (rootComponent == "camera")
					{
						ParseCameraCommand(comp, args);
					}
					else if (rootComponent == "bg")
					{
						ParseBgCommand(comp, args);
					}
					else if (rootComponent == "images")
					{
						ParseImageMapCommand(comp, args);
					}
					else if (rootComponent == "anims")
					{
						if (m_panimmanager == NULL)
							ThrowParseException("Animation manager not available", rootComponent);
						ParseAnimationCommand(comp, args);
					}
					else
					{
						if (!ParsePrimitiveCommand(rootComponent, comp, args) &&
							!ParseArrayCommand(rootComponent, comp, args))
						{
							CBaseObj* pobj = NULL;
							if (root[0] == '.')
							{
								pobj = DrillDown(&m_pmodel->GetSuperGroup(), rootComponent, comp);
							}
							else
							{
								pobj = FindObjInEnvVars(rootComponent);
								if (pobj != NULL && pobj->GetType() == ObjType::Group && !comp.empty())
								{
									string rootComponent = comp[0];
									comp.erase(comp.begin());

									pobj = DrillDown((CGrp*)pobj, rootComponent, comp);
								}
							}
							if (pobj == NULL)
								ThrowParseException("Bad root component", rootComponent);

							if (comp.size() > 0)
							{
								switch (pobj->GetType())
								{
								case ObjType::Group:
									ParseGroupCommand((CGrp*)pobj, comp, args);
									break;
								case ObjType::DirLight:
								case ObjType::PtLight:
								case ObjType::SpotLight:
									ParseLightCommand((CLight*)pobj, comp, args);
									break;
								case ObjType::Sphere:
									ParseSphereCommand((CSphere*)pobj, comp, args);
									break;
								case ObjType::Polygon:
									ParsePolygonCommand((CPolygon*)pobj, comp, args);
									break;
								}
							}
							else
							{
								switch (pobj->GetType())
								{
								case ObjType::Group: AssignReturnValue<>((CGrp*)pobj); break;
								case ObjType::DirLight: AssignReturnValue<>((CDirLight*)pobj); break;
								case ObjType::PtLight: AssignReturnValue<>((CPtLight*)pobj); break;
								case ObjType::SpotLight: AssignReturnValue<>((CSpotLight*)pobj); break;
								case ObjType::Sphere: AssignReturnValue<>((CSphere*)pobj); break;
								case ObjType::Polygon: AssignReturnValue<>((CPolygon*)pobj); break;
								}
							}
						}
					}
				}
				else
					ThrowParseException("Invalid command", root);
			}

			if (!m_returnVarName.empty())
				ThrowParseException("Unable to assign return var, command did not return a value", root);
		}
	}
	catch (const mig_exception& e)
	{
		m_serr = FormatError(m_script, m_line, e.what());
		return false;
	}

	return true;
}

template <typename T>
bool CParser2::SetEnvVar(const string& name, const T& value)
{
	try
	{
		map<string, T>& mapVars = GetVarMap<T>();
		mapVars[name] = value;
	}
	catch (const parse_exception& e)
	{
		m_serr = e.what();
		return false;
	}

	return true;
}

template <typename T>
bool CParser2::GetEnvVar(const string& name, T& value)
{
	try
	{
		map<string, T>& mapVars = GetVarMap<T>();
		value = mapVars[name];
	}
	catch (const parse_exception& e)
	{
		m_serr = e.what();
		return false;
	}

	return true;
}

template bool CParser2::SetEnvVar(const string& name, const int& value);
template bool CParser2::GetEnvVar(const string& name, int& value);
template bool CParser2::SetEnvVar(const string& name, const double& value);
template bool CParser2::GetEnvVar(const string& name, double& value);
template bool CParser2::SetEnvVar(const string& name, const bool& value);
template bool CParser2::GetEnvVar(const string& name, bool& value);
template bool CParser2::SetEnvVar(const string& name, const string& value);
template bool CParser2::GetEnvVar(const string& name, string& value);
template bool CParser2::SetEnvVar(const string& name, const COLOR& value);
template bool CParser2::GetEnvVar(const string& name, COLOR& value);
template bool CParser2::SetEnvVar(const string& name, const UVC& value);
template bool CParser2::GetEnvVar(const string& name, UVC& value);
template bool CParser2::SetEnvVar(const string& name, const CPt& value);
template bool CParser2::GetEnvVar(const string& name, CPt& value);
template bool CParser2::SetEnvVar(const string& name, const CMatrix& value);
template bool CParser2::GetEnvVar(const string& name, CMatrix& value);
template bool CParser2::SetEnvVar(const string& name, const CUnitVector& value);
template bool CParser2::GetEnvVar(const string& name, CUnitVector& value);

template <typename T>
bool CParser2::SetEnvVarModelRef(const string& name, T* value)
{
	try
	{
		map<string, T*>& mapVars = GetVarMap<T*>();
		mapVars[name] = value;
	}
	catch (const parse_exception& e)
	{
		m_serr = e.what();
		return false;
	}

	return true;
}

template <typename T>
bool CParser2::GetEnvVarModelRef(const string& name, T*& value)
{
	try
	{
		map<string, T*>& mapVars = GetVarMap<T*>();
		value = mapVars[name];
	}
	catch (const parse_exception& e)
	{
		m_serr = e.what();
		return false;
	}

	return true;
}

template bool CParser2::SetEnvVarModelRef(const string& name, CGrp* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CGrp*& value);
template bool CParser2::SetEnvVarModelRef(const string& name, CSphere* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CSphere*& value);
template bool CParser2::SetEnvVarModelRef(const string& name, CPolygon* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CPolygon*& value);
template bool CParser2::SetEnvVarModelRef(const string& name, CDirLight* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CDirLight*& value);
template bool CParser2::SetEnvVarModelRef(const string& name, CPtLight* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CPtLight*& value);
template bool CParser2::SetEnvVarModelRef(const string& name, CSpotLight* value);
template bool CParser2::GetEnvVarModelRef(const string& name, CSpotLight*& value);

bool CParser2::ParseCommandString(const string& line)
{
	m_serr.clear();
	m_line = 0;

	if (m_pmodel == NULL)
	{
		m_serr = FormatError(m_script, m_line, "Model not set, cannot parse");
		return false;
	}

	return ParseCommandStringInner(line);
}

bool CParser2::ParseCommandScript(istream& stream)
{
	m_line = 0;

	string buffer;
	while (getline(stream, buffer))
	{
		m_line++;

		if (buffer.empty())
			continue;

		if (!ParseCommandStringInner(buffer))
			break;
	}
	return m_serr.empty();
}

bool CParser2::ParseCommandScript(const string& path)
{
	if (m_pmodel == NULL)
	{
		m_serr = FormatError(m_script, m_line, "Model not set, cannot parse");
		return false;
	}
	m_serr.clear();

	fstream infile;
	infile.open(path, ios_base::in);
	if (!infile.is_open())
	{
		infile.open(m_envpath + path, ios_base::in);
		if (!infile.is_open())
		{
			m_serr = FormatError(m_script, m_line, "Failed to open script file: " + path);
			return false;
		}
	}
	m_script = path;

	size_t endOfPath = path.rfind('\\');
	if (endOfPath == string::npos)
		endOfPath = path.rfind('/');
	if (endOfPath != string::npos)
		m_envpath = path.substr(0, endOfPath + 1);

	bool ret = ParseCommandScript(infile);
	infile.close();
	return ret;
}

bool CParser2::ParseCommandScriptString(const string& script)
{
	if (m_pmodel == NULL)
	{
		m_serr = FormatError(m_script, m_line, "Model not set, cannot parse");
		return false;
	}
	m_serr.clear();

	stringstream strstream(script);
	m_script = "<embedded>";
	return ParseCommandScript(strstream);
}

const string& CParser2::GetError() const
{
	return m_serr;
}
