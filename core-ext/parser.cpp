// parser.cpp - defines a parser that generates models from scripts
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <string.h>

#include "../core/package.h"
#include "../core/migutil.h"
#include "parser.h"
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
	if (!token.empty())
		throw parse_exception("[" + token + "] " + message);
	throw parse_exception(message);
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
	return (c == 0 || c == ' ' || c == ',' || c == '\t' || c == '\r');
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

static string NextInList(vector<string>& args)
{
	string next = args.front();
	args.erase(args.begin());
	return next;
}

//-----------------------------------------------------------------------------
// CParser
//-----------------------------------------------------------------------------

CParser::CParser()
{
	m_pmodel = NULL;
	m_panimmanager = NULL;
	m_aspect = 1.333;
	m_line = 0;
}

CParser::CParser(CModel* pmodel)
{
	Init(pmodel);
}

CParser::CParser(CModel* pmodel, CAnimManager* panimmanager)
{
	Init(pmodel, panimmanager);
}

CParser::CParser(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo)
{
	Init(pmodel, panimmanager, rinfo);
}

CParser::~CParser()
{
	m_pmodel = NULL;
}

bool CParser::Init(CModel* pmodel)
{
	return Init(pmodel, NULL);
}

bool CParser::Init(CModel* pmodel, CAnimManager* panimmanager)
{
	m_pmodel = pmodel;
	if (m_pmodel != NULL)
		SetEnvVarModelRef<CGrp>("super", &m_pmodel->GetSuperGroup());
	m_panimmanager = panimmanager;
	m_aspect = 1.333;
	m_aspectIsFixed = false;
	return true;
}

bool CParser::Init(CModel* pmodel, CAnimManager* panimmanager, const REND_INFO& rinfo)
{
	if (!Init(pmodel, panimmanager))
		return false;
	m_aspect = (rinfo.height > 0 ? rinfo.width / (double)rinfo.height : 0);
	m_aspectIsFixed = true;
	return true;
}

void CParser::SetEnvPath(const string& envpath)
{
	m_envpath = envpath;
}

void CParser::ThrowParseException(const string& msg, const string& token)
{
	ThrowParseException(msg, token, "");
}

void CParser::ThrowParseException(const string& msg, const string& token, const string& help)
{
	if (!token.empty())
		throw parse_exception("[" + token + "] " + msg, help, m_script, m_line);
	throw parse_exception(msg, help, m_script, m_line);
}

void CParser::CheckArgs(const vector<string>& args, size_t minCount, const string& cmd, bool canEcho, const string& help)
{
	if (m_helpActive)
		ThrowParseException("", "", help);
	if (m_echoActive && !canEcho)
		ThrowParseException("This command does not support echo", "", help);
	if (!m_echoActive && args.size() < minCount)
		ThrowParseException("Arg count failed, expected " + to_string(minCount), cmd, help);
}

// getters for the variable maps
//

template <typename T>
map<string, T>& CParser::GetVarMap()
{
	throw parse_exception("Unknown map type");
}

template <typename T>
bool CParser::InVarMap(const string& name)
{
	map<string, T>& mapObjs = GetVarMap<T>();
	return (mapObjs.find(name) != mapObjs.end());
}

template <typename T>
T& CParser::GetFromVarMap(const string& name)
{
	return GetVarMap<T>()[name];
}

template <typename T>
void CParser::CreateNewVar(const string& name)
{
	GetVarMap<T>()[name];
}

template <> void CParser::CreateNewVar<CGrp*>(const string& name)
{
	CGrp* newGroup = m_pmodel->GetSuperGroup().CreateSubGroup();
	GetVarMap<CGrp*>()[name] = newGroup;
}

template <> void CParser::CreateNewVar<CSphere*>(const string& name)
{
	CSphere* newSphere = m_pmodel->GetSuperGroup().CreateSphere();
	GetVarMap<CSphere*>()[name] = newSphere;
}

template <> void CParser::CreateNewVar<CPolygon*>(const string& name)
{
	CPolygon* newPoly = m_pmodel->GetSuperGroup().CreatePolygon();
	GetVarMap<CPolygon*>()[name] = newPoly;
}

template <> void CParser::CreateNewVar<CDirLight*>(const string& name)
{
	CDirLight* newLight = m_pmodel->GetSuperGroup().CreateDirectionalLight();
	GetVarMap<CDirLight*>()[name] = newLight;
}

template <> void CParser::CreateNewVar<CPtLight*>(const string& name)
{
	CPtLight* newLight = m_pmodel->GetSuperGroup().CreatePointLight();
	GetVarMap<CPtLight*>()[name] = newLight;
}

template <> void CParser::CreateNewVar<CSpotLight*>(const string& name)
{
	CSpotLight* newLight = m_pmodel->GetSuperGroup().CreateSpotLight();
	GetVarMap<CSpotLight*>()[name] = newLight;
}

template <typename T> void CParser::AssignReturnValue(T value)
{
	if (!m_returnVarName.empty())
	{
		//CreateNewVar<T>(m_returnVarName);
		GetVarMap<T>()[m_returnVarName] = value;

		m_returnVarName.clear();
	}
}

template <typename T>
bool CParser::CopyEnvVarToNewParser(const string& name, CParser& newParser)
{
	bool returnValue = InVarMap<T>(name);
	if (returnValue)
		newParser.SetEnvVar<T>(name, GetFromVarMap<T>(name));
	return returnValue;
}

template <typename T>
bool CParser::CopyReturnValueFromOtherParser(CParser& other)
{
	bool returnValue = other.InVarMap<T>(other.m_scriptReturnVarName);
	if (returnValue)
		AssignReturnValue<T>(other.GetFromVarMap<T>(other.m_scriptReturnVarName));
	return returnValue;
}

// parsers for the flags
//

static const map<string, dword>& GetObjectFlags(void)
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
	return mapFlags;
}

static const map<string, dword>& GetPolyCurveFlags(void)
{
	static map<string, dword> mapFlags = {
		{"none", PCF_NONE},
		{"newplane", PCF_NEWPLANE},
		{"cull", PCF_CULL},
		{"invisible", PCF_INVISIBLE},
		{"cw", PCF_CW},
		{"vertexnorms", PCF_VERTEXNORMS}
	};
	return mapFlags;
}

static const map<string, dword>& GetTexturingFlags(void)
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
	return mapFlags;
}

static const map<string, dword>& GetRenderFlags(void)
{
	static map<string, dword> mapFlags = {
		{"none", REND_NONE},
		{"reflect", REND_AUTO_REFLECT},
		{"refract", REND_AUTO_REFRACT},
		{"shadows", REND_AUTO_SHADOWS},
		{"softshadows", REND_SOFT_SHADOWS},
		{"all", REND_ALL}
	};
	return mapFlags;
}

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

static string FlagsToString(dword flags, map<string, dword> mapFlags)
{
	string output = "";
	for (const auto& iter : mapFlags)
	{
		if (iter.first != "all" && iter.second & flags)
		{
			if (!output.empty())
				output += "|";
			output += iter.first;
		}
	}
	if (output.empty())
		output = "none";
	return "(" + output + ")";
}

dword CParser::ParseObjectFlags(const string& token)
{
	return ParseFlags(token, OBJF_NONE, GetObjectFlags());
}

dword CParser::ParsePolyCurveFlags(const string& token)
{
	return ParseFlags(token, PCF_NONE, GetPolyCurveFlags());
}

dword CParser::ParseTexturingFlags(const string& token)
{
	return ParseFlags(token, TXTF_NONE, GetTexturingFlags());
}

dword CParser::ParseRenderFlags(const string& token)
{
	return ParseFlags(token, REND_NONE, GetRenderFlags());
}

string CParser::ObjectFlagsToString(dword flags)
{
	return FlagsToString(flags, GetObjectFlags());
}

string CParser::PolyCurveFlagsToString(dword flags)
{
	return FlagsToString(flags, GetPolyCurveFlags());
}

string CParser::TexturingFlagsToString(dword flags)
{
	return FlagsToString(flags, GetTexturingFlags());
}

string CParser::RenderFlagsToString(dword flags)
{
	return FlagsToString(flags, GetRenderFlags());
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
T GetEnumMap(const string& enumStr, map<string, T>& theMap)
{
	ThrowParseException("Unknown enum type", enumStr);
}

template <> SuperSample GetEnumMap(const string& token, map<string, SuperSample>& theMap)
{
	theMap = {
		{"none", SuperSample::None},
		{"1x", SuperSample::X1},
		{"5x", SuperSample::X5},
		{"9x", SuperSample::X9},
		{"edge", SuperSample::Edge},
		{"object", SuperSample::Object}
	};
	return SuperSample::X1;
}

template <> ImageResize GetEnumMap(const string& token, map<string, ImageResize>& theMap)
{
	theMap = {
		{"none", ImageResize::None},
		{"stretch", ImageResize::Stretch},
		{"fit", ImageResize::ScaleToFit},
		{"fill", ImageResize::ScaleToFill}
	};
	return ImageResize::Stretch;
}

template <> ImageFormat GetEnumMap(const string& token, map<string, ImageFormat>& theMap)
{
	theMap = {
		{"none", ImageFormat::None},
		{"rgba", ImageFormat::RGBA},
		{"bgra", ImageFormat::BGRA},
		{"rgb", ImageFormat::RGB},
		{"bgr", ImageFormat::BGR},
		{"greyscale", ImageFormat::GreyScale},
		{"bump", ImageFormat::Bump}
	};
	return ImageFormat::None;
}

template <> TextureFilter GetEnumMap(const string& token, map<string, TextureFilter>& theMap)
{
	theMap = {
		{"none", TextureFilter::None},
		{"nearest", TextureFilter::Nearest},
		{"bilinear", TextureFilter::Bilinear}
	};
	return TextureFilter::None;
}

template <> TextureMapType GetEnumMap(const string& token, map<string, TextureMapType>& theMap)
{
	theMap = {
		{"none", TextureMapType::None},
		{"diffuse", TextureMapType::Diffuse},
		{"specular", TextureMapType::Specular},
		{"reflection", TextureMapType::Reflection},
		{"refraction", TextureMapType::Refraction},
		{"glow", TextureMapType::Glow},
		{"transparency", TextureMapType::Transparency},
		{"bump", TextureMapType::Bump}
	};
	return TextureMapType::None;
}

template <> TextureMapOp GetEnumMap(const string& token, map<string, TextureMapOp>& theMap)
{
	theMap = {
		{"multiply", TextureMapOp::Multiply},
		{"add", TextureMapOp::Add},
		{"subtract", TextureMapOp::Subtract},
		{"blend", TextureMapOp::Blend}
	};
	return TextureMapOp::Multiply;
}

template <> TextureMapWrapType GetEnumMap(const string& token, map<string, TextureMapWrapType>& theMap)
{
	theMap = {
		{"none", TextureMapWrapType::None},
		{"spherical", TextureMapWrapType::Spherical},
		{"elliptical", TextureMapWrapType::Elliptical},
		{"projection", TextureMapWrapType::Projection},
		{"extrusion", TextureMapWrapType::Extrusion}
	};
	return TextureMapWrapType::None;
}

template <> AnimType GetEnumMap(const string& token, map<string, AnimType>& theMap)
{
	theMap = {
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
	return AnimType::None;
}

template <> AnimInterpolation GetEnumMap(const string& token, map<string, AnimInterpolation>& theMap)
{
	theMap = {
		{"spline", AnimInterpolation::Spline},
		{"spline2", AnimInterpolation::Spline2},
		{"linear", AnimInterpolation::Linear}
	};
	return AnimInterpolation::Linear;
}

template <> AnimOperation GetEnumMap(const string& token, map<string, AnimOperation>& theMap)
{
	theMap = {
		{"multiply", AnimOperation::Multiply},
		{"sum", AnimOperation::Sum},
		{"replace", AnimOperation::Replace}
	};
	return AnimOperation::Replace;
}

template <typename T>
static T CParser::ParseEnum(const string& enumStr)
{
	static map<string, T> mapFlags;
	T defValue = GetEnumMap<T>(enumStr, mapFlags);
	return LookupEnum(enumStr, mapFlags, defValue);
}

template <typename T>
static string CParser::EnumToString(T value)
{
	static map<string, T> mapFlags;
	GetEnumMap<T>("", mapFlags);
	for (const auto& iter : mapFlags)
	{
		if (iter.second == value)
			return iter.first;
	}
	return "???";
}

template <typename T>
static string CParser::Stringify(const T& obj)
{
	return to_string(obj);
	//throw parse_exception("Unable to stringify");
}

template <> string CParser::Stringify(const string& obj)
{
	return obj;
}

template <> string CParser::Stringify(const COLOR& obj)
{
	return "[r=" + to_string(obj.r) +
		", g=" + to_string(obj.b) +
		", b=" + to_string(obj.b) +
		", a=" + to_string(obj.a) + "]";
}

template <> string CParser::Stringify(const UVC& obj)
{
	return "[u=" + to_string(obj.u) + ", v=" + to_string(obj.v) + "]";
}

template <> string CParser::Stringify(const CPt& obj)
{
	return "[x=" + to_string(obj.x) + ", y=" + to_string(obj.y) + ", z=" + to_string(obj.z) + "]";
}

template <> string CParser::Stringify(const CUnitVector& obj)
{
	return "[x=" + to_string(obj.x) + ", y=" + to_string(obj.y) + ", z=" + to_string(obj.z) + "]";
}

template <> string CParser::Stringify(const CMatrix& obj)
{
	const double* ctm = obj.GetCTM();
	return "[" + to_string(ctm[0]) + ", " + to_string(ctm[1]) + ", " + to_string(ctm[2]) + ", " + to_string(ctm[3]) + "]\n" +
		"[" + to_string(ctm[4]) + ", " + to_string(ctm[5]) + ", " + to_string(ctm[6]) + ", " + to_string(ctm[7]) + "]\n" +
		"[" + to_string(ctm[8]) + ", " + to_string(ctm[9]) + ", " + to_string(ctm[10]) + ", " + to_string(ctm[11]) + "]\n" +
		"[" + to_string(ctm[12]) + ", " + to_string(ctm[13]) + ", " + to_string(ctm[14]) + ", " + to_string(ctm[15]) + "]\n";
}

template <> string CParser::Stringify(const CImageBuffer& obj)
{
	int w, h;
	obj.GetSize(w, h);
	return "[path='" + string(obj.GetPath()) + "', w=" + to_string(w) + ", h=" + to_string(h) + ", fmt=" + EnumToString(obj.GetFormat()) + "]\n";
}

template <> string CParser::Stringify(const CAnimRecord& obj)
{
	return "[obj=" + obj.GetObjectName() +
		", type=" + EnumToString(obj.GetAnimType()) +
		", op=" + EnumToString(obj.GetOperation()) +
		", int=" + EnumToString(obj.GetInterpolation()) +
		", b=" + to_string(obj.GetBeginTime()) +
		", e=" + to_string(obj.GetEndTime()) + "]";
}

template <> string CParser::Stringify(const CGrp& obj)
{
	return "[name=" + obj.GetName() +
		", grps=" + to_string(obj.GetSubGroups().size()) +
		", objs=" + to_string(obj.GetObjects().size()) +
		", lits=" + to_string(obj.GetLights().size()) + "]";
}

template <> string CParser::Stringify(const CLight& obj)
{
	return "[name=" + obj.GetName() +
		", type=" + (obj.GetType() == ObjType::PtLight ? "point" : (obj.GetType() == ObjType::DirLight ? "dir" : "spot")) +
		", color=" + Stringify(obj.GetColor()) +
		", shadow=" + (obj.IsShadowCaster() ? "yes" : "no") + "]";
}

inline string EchoIfFound(const TEXTURES& textures, const string& prefix)
{
	return (!textures.empty()) ? (", " + prefix + "=" + to_string(textures.size())) : "";
}

template <> string CParser::Stringify(const CPolygon& obj)
{
	return "[name=" + obj.GetName() +
		", lats=" + to_string(obj.GetLattice().Count()) +
		", crvs=" + to_string(obj.GetCurves().Count()) +
		", inds=" + to_string(obj.GetIndices().Count()) +
		", nrms=" + to_string(obj.GetNormals().Count()) + 
		EchoIfFound(obj.GetTextures(TextureMapType::Diffuse), "dmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Specular), "smaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Refraction), "rfrmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Glow), "gmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Reflection), "rflmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Transparency), "tmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Bump), "bmaps") +
		"]";
}

template <> string CParser::Stringify(const CSphere& obj)
{
	return "[name=" + obj.GetName() +
		", origin=" + Stringify(obj.GetOrigin()) +
		", radius=" + to_string(obj.GetRadius()) +
		EchoIfFound(obj.GetTextures(TextureMapType::Diffuse), "dmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Specular), "smaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Refraction), "rfrmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Glow), "gmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Reflection), "rflmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Transparency), "tmaps") +
		EchoIfFound(obj.GetTextures(TextureMapType::Bump), "bmaps") +
		"]";
}

// parsers for the basic object types, not using the maps
//

template <typename T>
T CParser::ParseObject(const string& token)
{
	ThrowParseException("Unknown parsing type", token);
}

#include <algorithm>

template <> int CParser::ParseObject(const string& token)
{
	//if (!all_of(token.begin(), token.end(), ::isdigit))
	//	ThrowParseException("Invalid integer string", token);
	//return atoi(token.c_str());
	char* p;
	int i = strtol(token.c_str(), &p, 10);
	if (*p != 0)
		ThrowParseException("Invalid int string", token);
	return i;
}

template <> double CParser::ParseObject(const string& token)
{
	//return atof(token.c_str());
	char* p;
	double d = strtod(token.c_str(), &p);
	if (*p != 0)
		ThrowParseException("Invalid double string", token);
	return d;
}

template <> bool CParser::ParseObject(const string& token)
{
	if (token == "true")
		return true;
	else if (token == "false")
		return false;
	throw parse_exception("Error parsing bool: " + token);
}

template <> string CParser::ParseObject(const string& token)
{
	size_t lastindex = token.size();
	if (token[0] != '\"' || token[lastindex - 1] != '\"')
		return token;

	return token.substr(1, lastindex - 2);
}

template <> SuperSample CParser::ParseObject(const string& token)
{
	return ParseEnum<SuperSample>(token);
}

template <> ImageResize CParser::ParseObject(const string& token)
{
	return ParseEnum<ImageResize>(token);
}

template <> ImageFormat CParser::ParseObject(const string& token)
{
	return ParseEnum<ImageFormat>(token);
}

template <> TextureFilter CParser::ParseObject(const string& token)
{
	return ParseEnum<TextureFilter>(token);
}

template <> TextureMapType CParser::ParseObject(const string& token)
{
	return ParseEnum<TextureMapType>(token);
}

template <> TextureMapOp CParser::ParseObject(const string& token)
{
	return ParseEnum<TextureMapOp>(token);
}

template <> TextureMapWrapType CParser::ParseObject(const string& token)
{
	return ParseEnum<TextureMapWrapType>(token);
}

template <> COLOR CParser::ParseObject(const string& token)
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

template <> UVC CParser::ParseObject(const string& token)
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

template <> CPt CParser::ParseObject(const string& token)
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

template <> CUnitVector CParser::ParseObject(const string& token)
{
	CPt pt = ParseObject<CPt>(token);
	return CUnitVector(pt);
}

// parsers for the basic object types, using the maps
//

template <typename T>
T CParser::ParseObjectCached(const string& name)
{
	map<string, T>& mapCache = GetVarMap<T>();
	if (mapCache.find(name) != mapCache.end())
		return mapCache[name];
	else
		return ParseObject<T>(name);
}

template <> CGrp* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CGrp*) DrillDown(name, ObjType::Group);
	else
	{
		CGrp* pgrp = NULL;

		vector<string> comp;
		if (SplitNestedComponents(name, comp) > 0)
		{
			string root = NextInList(comp);

			pgrp = GetFromVarMap<CGrp*>(root);
			if (pgrp == NULL)
				ThrowParseException("Error parsing object", name);

			if (!comp.empty())
			{
				root = NextInList(comp);

				CBaseObj* pobj = DrillDown(pgrp, root, comp);
				if (pobj == NULL || pobj->GetType() != ObjType::Group)
					ThrowParseException("Error parsing object", name);
				pgrp = (CGrp*)pobj;
			}
		}
		return pgrp;
	}
}

template <> CDirLight* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CDirLight*)DrillDown(name, ObjType::DirLight);
	else
		return GetFromVarMap<CDirLight*>(name);
}

template <> CPtLight* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CPtLight*)DrillDown(name, ObjType::PtLight);
	else
		return GetFromVarMap<CPtLight*>(name);
}

template <> CSpotLight* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CSpotLight*)DrillDown(name, ObjType::SpotLight);
	else
		return GetFromVarMap<CSpotLight*>(name);
}

template <> CSphere* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CSphere*)DrillDown(name, ObjType::Sphere);
	else
		return GetFromVarMap<CSphere*>(name);
}

template <> CPolygon* CParser::ParseObjectCached(const string& name)
{
	if (name[0] == '.')
		return (CPolygon*)DrillDown(name, ObjType::Polygon);
	else
		return GetFromVarMap<CPolygon*>(name);
}

template <> CBaseObj* CParser::ParseObjectCached(const string& name)
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
		string root = NextInList(comp);

		// first element must be a group
		CGrp* pgrp = GetFromVarMap<CGrp*>(root);
		if (pgrp == NULL)
			ThrowParseException("Error parsing object", name);

		if (!comp.empty())
		{
			root = NextInList(comp);

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
T CParser::ParseObjectInvertible(const string& name)
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
unique_ptr<T> CParser::ParseObjectNullable(const string& token)
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
const CArray<T>& CParser::ParseArray(const string& token)
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

string CParser::GenerateObjectNamespace(const CGrp* parent, const CBaseObj* pobj) const
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

string CParser::GenerateObjectNamespace(const CGrp* parent, const string& name)
{
	CBaseObj* pobj = ParseObjectCached<CBaseObj*>(name);
	return GenerateObjectNamespace(&m_pmodel->GetSuperGroup(), pobj);
}

static size_t ExtractIndexFromName(const string& name)
{
	string indexStr = name.substr(3);
	return atoi(indexStr.c_str());
}

CBaseObj* CParser::DrillDown(CGrp* parent, string& next, vector<string>& comp)
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
			next = NextInList(comp);
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
			next = NextInList(comp);
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

CBaseObj* CParser::DrillDown(const string& name, ObjType expectedType)
{
	CBaseObj* pobj = m_pmodel->GetSuperGroup().DrillDown(name);
	if (pobj == NULL)
		ThrowParseException("Unable to drill into namespace", name);
	if (expectedType != ObjType::None && pobj->GetType() != expectedType)
		ThrowParseException("Object type mismatch", name);
	return pobj;
}

CBaseObj* CParser::FindObjInEnvVars(const string& name)
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
bool CParser::ParseCreateCommand(const string& typeName, const string& name, const string& type)
{
	if (type == typeName)
	{
		CreateNewVar<T>(name);
		return true;
	}
	return false;
}

template <typename T>
bool CParser::ParsePrimitiveCommand(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<T>(root))
	{
		T& value = GetFromVarMap<T>(root);
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Primitive commands: set");
			cout << Stringify(value) << "\n";
			return true;
		}

		if (command == "set")
		{
			CheckArgs(args, 1, command, false,
				"Usage: set [value]\nSets a new value\n");
			value = ParseObjectCached<T>(args[0]);
		}
		else
			ThrowParseException("Invalid primitive command", command);

		return true;
	}
	return false;
}

template <> bool CParser::ParsePrimitiveCommand<string>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<string>(root))
	{
		string& value = GetFromVarMap<string>(root);
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("String commands: set, append, toint, todouble");
			cout << value << "\n";
			return true;
		}

		if (command == "set")
		{
			CheckArgs(args, 1, command, false,
				"Usage: set [value]\nSets a new value\n");
			value = ParseObjectCached<string>(args[0]);
		}
		else if (command == "append")
		{
			CheckArgs(args, 1, command, false,
				"Usage: append [string]\nAppends a string\n");
			AssignReturnValue<>(value.append(ParseObjectCached<string>(args[0])));
		}
		else if (command == "toint")
		{
			CheckArgs(args, 0, command, false,
				"Usage: toint\nConverts the string to an int\n");
			AssignReturnValue<>(atoi(value.c_str()));
		}
		else if (command == "todouble")
		{
			CheckArgs(args, 0, command, false,
				"Usage: todouble\nConverts the string to a double\n");
			AssignReturnValue<>(atof(value.c_str()));
		}
		else
			ThrowParseException("Invalid string command", command);

		return true;
	}
	return false;
}

template <> bool CParser::ParsePrimitiveCommand<CPt>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CPt>(root))
	{
		CPt& pt = GetFromVarMap<CPt>(root);
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Point commands: set, scale, invert");
			cout << Stringify(pt) << "\n";
		}

		if (command == "set")
		{
			CheckArgs(args, 1, command, false,
				"Usage: set [value]\nSets a new value\n");
			pt = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "scale")
		{
			CheckArgs(args, 1, command, false,
				"Usage: scale [value]\nScales the point by a value\n");
			pt.Scale(ParseObjectInvertible<double>(args[0]));
		}
		else if (command == "invert")
		{
			CheckArgs(args, 0, command, false,
				"Usage: invert\nInverts the point\n");
			pt.Invert();
		}
		else
			ThrowParseException("Invalid point command", command);

		return true;
	}
	return false;
}

template <> bool CParser::ParsePrimitiveCommand<CUnitVector>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CUnitVector>(root))
	{
		CUnitVector& unitVector = GetFromVarMap<CUnitVector>(root);
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Unit vector commands: set, point, normalize");
			cout << Stringify(unitVector) << "\n";
		}

		if (command == "set")
		{
			CheckArgs(args, 1, command, false,
				"Usage: set [value]\nSets a new value\n");
			unitVector = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "point")
		{
			CheckArgs(args, 1, command, false,
				"Usage: point\nSets the unitvector to a point\n");
			unitVector = ParseObjectCached<CPt>(args[0]);
		}
		else if (command == "normalize")
		{
			CheckArgs(args, 0, command, false,
				"Usage: normalize\nNormalizes the unitvector\n");
			unitVector.Normalize();
		}
		else
			ThrowParseException("Invalid unit vector command", command);

		return true;
	}
	return false;
}

template <> bool CParser::ParsePrimitiveCommand<CMatrix>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CMatrix>(root))
	{
		ParseMatrixCommand(GetFromVarMap<CMatrix>(root), command, args);
		return true;
	}
	return false;
}

template <> bool CParser::ParsePrimitiveCommand<CAnimRecord>(const string& root, const string& command, vector<string>& args)
{
	if (InVarMap<CAnimRecord>(root))
	{
		CAnimRecord& record = GetFromVarMap<CAnimRecord>(root);
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Animation record commands: type, times, reverse, applytostart, operation, interpolation, bias, bias2, values");
			cout << Stringify(record) << "\n";
		}

		if (command == "type")
		{
			CheckArgs(args, 1, command, false,
				"Usage: type [type]\nSet the animation type (translate, scale, etc)\n");
			record.SetAnimType(ParseEnum<AnimType>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "times")
		{
			CheckArgs(args, 2, command, false,
				"Usage: times [begin] [end]\nSets the begin and end times for the animation\n");
			record.SetTimes(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
		}
		else if (command == "reverse")
		{
			CheckArgs(args, 1, command, false,
				"Usage: reverse [bool]\nIf true, animation record will run in reverse\n");
			record.SetReverse(ParseObjectCached<bool>(args[0]));
		}
		else if (command == "applytostart")
		{
			CheckArgs(args, 1, command, false,
				"Usage: applytostart [bool]\nIf true, starting value will be applied to the beginning of the complete animation\n");
			record.SetApplyToStart(ParseObjectCached<bool>(args[0]));
		}
		else if (command == "operation")
		{
			CheckArgs(args, 1, command, false,
				"Usage: operation [op]\nSets the operation type (replace, sum, multiply)\n");
			record.SetOperation(ParseEnum<AnimOperation>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "interpolation")
		{
			CheckArgs(args, 1, command, false,
				"Usage: interpolation [op]\nSets the interpolation type (linear, spline, spline2)\n");
			record.SetInterpolation(ParseEnum<AnimInterpolation>(ParseObjectCached<string>(args[0])));
		}
		else if (command == "bias")
		{
			CheckArgs(args, 1, command, false,
				"Usage: bias [value]\nSet the spline/spline2 bias\n");
			record.SetBias(ParseObjectCached<double>(args[0]));
		}
		else if (command == "bias2")
		{
			CheckArgs(args, 1, command, false,
				"Usage: bias2 [value]\nSet the spline2 second bias\n");
			record.SetBias2(ParseObjectCached<double>(args[0]));
		}
		else if (command == "values")
		{
			CheckArgs(args, 1, command, false,
				"Usage: values [begin] [end]\nSets the beginning and ending values\n");
			ParseAnimRecordValues(record, ParseObjectCached<string>(args[0]), args.size() > 1 ? ParseObjectCached<string>(args[1]) : "");
		}
		else
			ThrowParseException("Invalid animation record command", command);

		return true;
	}
	return false;
}

template <typename T>
bool CParser::ParseArrayCommand(const string& root, const string& command, vector<string>& args)
{
	map<string, CArray<T>>& mapArrays = GetVarMap<CArray<T>>();
	if (mapArrays.find(root) != mapArrays.end())
	{
		CArray<T>& objArray = mapArrays[root];
		if (command.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Array commands: set, add");
			cout << "[";
			const T* arr = objArray.ToArray();
			if (arr != nullptr)
			{
				for (int i = 0; i < objArray.GetSize(); i++)
				{
					if (i > 0)
						cout << ", ";
					cout << Stringify(arr[i]);
				}
			}
			cout << "]\n";
			return true;
		}

		if (command == "set")
		{
			CheckArgs(args, 1, command, false,
				"Usage: set [other]\nAssigns an array to a set of values\n");
			objArray = ParseArray<T>(args[0]);
		}
		else if (command == "add")
		{
			CheckArgs(args, 1, command, false,
				"Usage: add [other]\nAdds a set of values to the end of an array\n");
			objArray.Add(ParseObjectCached<T>(args[0]));
		}
		else
			ThrowParseException("Invalid array command", command);

		return true;
	}

	return false;
}

bool CParser::ParseCreateCommand(vector<string>& args)
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
				CheckArgs(args, 3, args[0], false,
					"Usage: var [name]:[type] = value\nCreates a new variable with a type, and sets it to another value\n");
				if (args[1] == "=")
				{
					string root = nameType[0];
					NextInList(args);
					NextInList(args);

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
			ThrowParseException("Invalid create command, or unknown type", args[0]);

		// we consumed the command completely
		return true;
	}
	else
	{
		CheckArgs(args, 3, args[0], false,
			"Usage: var [name] = value\nCreates a new variable, and sets it to another value\n");
		if (args[1] != "=")
			ThrowParseException("Invalid create command", args[0]);
		m_returnVarName = args[0];
		NextInList(args);
		NextInList(args);
	}

	return false;
}

bool CParser::ParsePrimitiveCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	string command = (!comps.empty() ? comps[0] : "");

	if (ParsePrimitiveCommand<int>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<double>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<bool>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<string>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<COLOR>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<UVC>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<CPt>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<CUnitVector>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<CMatrix>(root, command, args))
		return true;
	if (ParsePrimitiveCommand<CAnimRecord>(root, command, args))
		return true;

	return false;
}

bool CParser::ParseArrayCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	string command = (!comps.empty() ? comps[0] : "");

	if (ParseArrayCommand<int>(root, command, args))
		return true;
	if (ParseArrayCommand<double>(root, command, args))
		return true;
	if (ParseArrayCommand<string>(root, command, args))
		return true;
	if (ParseArrayCommand<COLOR>(root, command, args))
		return true;
	if (ParseArrayCommand<CPt>(root, command, args))
		return true;
	if (ParseArrayCommand<CUnitVector>(root, command, args))
		return true;
	if (ParseArrayCommand<UVC>(root, command, args))
		return true;

	return false;
}

void CParser::ParseScriptCommand(vector<string>& args)
{
	CParser parser(m_pmodel, m_panimmanager);
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
}

void CParser::ParseMatrixCommand(CMatrix& matrix, const string& command, vector<string>& args)
{
	if (command.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Matrix commands: identity, set, rotatex, rotatey, rotatez, translate, scale");
		cout << Stringify(matrix);
		return;
	}

	if (command == "identity")
	{
		CheckArgs(args, 0, command, false,
			"Usage: identity\nSets the matrix to the identity matrix\n");
		matrix.SetIdentity();
	}
	else if (command == "set")
	{
		CheckArgs(args, 1, command, false,
			"Usage: set [other]\nSets the matrix to another existing matrix\n");
		CMatrix& other = GetFromVarMap<CMatrix>(args[0]);
		matrix = other;
	}
	else if (command == "rotatex")
	{
		CheckArgs(args, 1, command, false,
			"Usage: rotatex [degrees]\nRotate the matrix around X\n");
		matrix.RotateX(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "rotatey")
	{
		CheckArgs(args, 1, command, false,
			"Usage: rotatey [degrees]\nRotate the matrix around Y");
		matrix.RotateY(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "rotatez")
	{
		CheckArgs(args, 1, command, false,
			"Usage: rotatez [degrees]\nRotate the matrix around Z");
		matrix.RotateZ(ParseObjectInvertible<double>(args[0]));
	}
	else if (command == "translate")
	{
		CheckArgs(args, 1, command, false,
			"Usage: translate [point]\nTranslate the matrix by a point\n");

		CPt vector = ParseObjectCached<CPt>(args[0]);
		matrix.Translate(vector.x, vector.y, vector.z);
	}
	else if (command == "scale")
	{
		CheckArgs(args, 1, command, false,
			"Usage: scale [point]\nScales the matrix by a point\n");

		CPt vector = ParseObjectCached<CPt>(args[0]);
		matrix.Scale(vector.x, vector.y, vector.z);
	}
	else
		ThrowParseException("Invalid matrix command", command);
}

void CParser::ParseMatrixCommand(CMatrix& matrix, const vector<string>& comps, vector<string>& args)
{
	ParseMatrixCommand(matrix, comps.empty() ? "" : comps[0], args);
}

void CParser::ParseCameraCommand(vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
		throw parse_exception("Camera commands: matrix, aspect, viewport");
	string first = NextInList(comps);

	if (first == "matrix")
	{
		ParseMatrixCommand(m_pmodel->GetCamera().GetTM(), comps, args);
	}
	else if (first == "aspect")
	{
		if (!m_aspectIsFixed)
		{
			CheckArgs(args, 1, first, true,
				"Usage: aspect [ratio]\nChanges the camera aspect ratio\n");
			if (!m_echoActive)
				m_aspect = ParseObjectCached<double>(args[0]);
			else
				cout << to_string(m_aspect) << "\n";
		}
	}
	else if (first == "viewport")
	{
		CheckArgs(args, 2, first, true,
			"Usage: viewport [minvp] [dist] [aspect]\nChanges the camera viewport\n" \
			" [minvp]: front clipping plane\n [dist]: distance to viewport\n [aspect]: optional aspect ratio\n");

		if (!m_echoActive)
		{
			double minvp = ParseObjectInvertible<double>(args[0]);
			double dist = ParseObjectInvertible<double>(args[1]);
			double aspect = (args.size() > 2 && m_aspect == 0 ? ParseObjectCached<double>(args[2]) : m_aspect);

			double ulen = (aspect > 1 ? minvp * aspect : minvp);
			double vlen = (aspect < 1 ? minvp / aspect : minvp);
			m_pmodel->GetCamera().SetViewport(ulen, vlen, dist);
		}
		else
		{
			double ulen, vlen, dist;
			m_pmodel->GetCamera().GetViewport(ulen, vlen, dist);
			cout << "ulen=" << to_string(ulen) << ", vlen=" << to_string(vlen) << ", dist=" << to_string(dist) << "\n";
		}
	}
	else
		ThrowParseException("Invalid camera command", first);
}

void CParser::ParseBgCommand(vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
		throw parse_exception("Background commands: colors, image");
	string first = NextInList(comps);

	if (first == "colors")
	{
		CheckArgs(args, 3, first, true,
			"Usage: colors [n] [e] [s]\nSets the background colors\n [n]: north color\n [e]: equator color\n [s]: south color\n");

		if (!m_echoActive)
		{
			COLOR n = ParseObjectCached<COLOR>(args[0]);
			COLOR e = ParseObjectCached<COLOR>(args[1]);
			COLOR s = ParseObjectCached<COLOR>(args[2]);
			m_pmodel->GetBackgroundHandler().SetBackgroundColors(n, e, s);
		}
		else
		{
			COLOR n, e, s;
			m_pmodel->GetBackgroundHandler().GetBackgroundColors(n, e, s);
			cout << "n=" << Stringify(n) << "\ne=" << Stringify(e) << "\ns=" << Stringify(s) << "\n";
		}
	}
	else if (first == "image")
	{
		CheckArgs(args, 2, first, true,
			"Usage: image [path] [isize]\nSets the background image\n [path]: path to image\n [isize]: stretch, fit, fill\n");

		if (!m_echoActive)
		{
			string path = ParseObjectCached<string>(args[0]);
			ImageResize isize = ParseObject<ImageResize>(args[1]);
			m_pmodel->GetBackgroundHandler().SetBackgroundImage(path, isize);
		}
		else
		{
			string path;
			ImageResize isize;
			m_pmodel->GetBackgroundHandler().GetBackgroundImage(path, isize);
			cout << "path='" << path << "', isize=" << EnumToString(isize) << "\n";
		}
	}
	else
		ThrowParseException("Invalid background command", first);
}

void CParser::ParseImageMapCommand(vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Image map commands: load, deleteall");
		for (const auto& iter : m_pmodel->GetImageMap().GetBufferMap())
		{
			if (iter.second)
				cout << iter.first << " -> " << Stringify(*iter.second);
		}
		return;
	}
	string first = NextInList(comps);

	if (first == "load")
	{
		CheckArgs(args, 1, first, false,
			"Usage: load [path] [fmt] title]\nLoads an image into the image map\n" \
			" [path]: path to image\n [fmt]: optional image format\n [title]: optional title to assign to image\n");
		string path = ParseObjectCached<string>(args[0]);
		ImageFormat fmt = (args.size() > 1 ? ParseObject<ImageFormat>(args[1]) : ImageFormat::None);
		string title = (args.size() > 2 ? ParseObjectCached<string>(args[2]) : "");

		title = m_pmodel->GetImageMap().LoadImageFile(CEnv::GetEnv().ExpandPathVars(path), fmt, title);
		if (title.empty())
			ThrowParseException("Bad path", path);
		AssignReturnValue<>(title);
	}
	else if (first == "deleteall")
	{
		CheckArgs(args, 0, first, false,
			"Usage: deleteall\nDeletes all loaded images\n");
		m_pmodel->GetImageMap().DeleteAll();
	}
	else
	{
		first = ParseObjectCached<string>(first);
		if (!ParseImagesCommand(first, comps, args))
			ThrowParseException("Invalid image map command", first);
	}
}

bool CParser::ParseImagesCommand(const string& root, vector<string>& comps, vector<string>& args)
{
	CImageBuffer* pimage = m_pmodel->GetImageMap().GetImage(root);
	if (pimage != NULL)
	{
		if (comps.empty())
		{
			if (!m_echoActive)
				throw parse_exception("Image commands: fillalphafromcolors, fillalphafromimage, fillalphafromtransparentcolor, normalizealpha");
			cout << Stringify(*pimage);
			return true;
		}
		string command = comps[0];

		if (command == "fillalphafromcolors")
		{
			CheckArgs(args, 0, command, false,
				"Usage: fillalphafromcolors\nFills the image alpha channel using the average of the color values\n");
			AssignReturnValue<>(pimage->FillAlphaFromColors());
		}
		else if (command == "fillalphafromimage")
		{
			CheckArgs(args, 1, command, false,
				"Usage: fillalphafromimage [other]\nFills the image alpha channel using another image\n [other]: title of other loaded image\n");
			string other = ParseObjectCached<string>(args[0]);
			CImageBuffer* pother = m_pmodel->GetImageMap().GetImage(other);
			if (pother != NULL)
				AssignReturnValue<>(pimage->FillAlphaFromImage(*pother));
			else
				ThrowParseException("Unknown image", other);
		}
		else if (command == "fillalphafromtransparentcolor")
		{
			CheckArgs(args, 7, command, false,
				"Usage: fillalphafromtransparentcolor [r] [g] [b] [dr] [dg] [db] [a]\nReplaces alpha using color filters\n" \
				" [r] [g] [b]: target color\n [dr] [dg] [db]: deltas\n [a]: alpha replacement value\n");
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
			CheckArgs(args, 0, command, false,
				"Usage: normalizealpha\nNormalizes the alpha channel to a full 0-255 range\n");
			pimage->NormalizeAlpha();
		}
		else
			ThrowParseException("Invalid image command", command);

		return true;
	}

	return false;
}

void CParser::ParseAnimRecordValues(CAnimRecord& record, const string& beginStr, const string& endStr)
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

void CParser::ParseAnimationCommand(vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Animation commands: add, load, newrecord");
		for (const auto& iter : m_panimmanager->GetList())
			cout << Stringify(iter) << "\n";
		return;
	}
	string first = NextInList(comps);

	if (first == "add")
	{
		CheckArgs(args, 2, first, false,
			"Usage: add [obj] [varOrType] [begin] [end] [beginValue] [endValue]\nAdds a new animation record\n" \
			" [obj]: full object name or constant\n [varOrType]: name of existing record, or type\n" \
			" [begin] [end]: begin/end times\n [beginValue]: begin value\n [endValue]: optional end value\n");
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
		CheckArgs(args, 2, first, false,
			"Usage: load [obj] [path] [begin] [end] [applyToStart] [reverse]\nLoads an animation JSON record\n" \
			" [obj]: full object name or constant\n [path]: path to JSON record\n [begin] [end]: begin/end times\n" \
			" [applyToStart]: optionally apply beginning value to start\n [reverse]: optionall reverse animation\n");
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

		CAnimRecord record = JsonReader::LoadFromJsonFile<CAnimRecord>(CEnv::GetEnv().ExpandPathVars(path), m_envpath);
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
		CheckArgs(args, 0, first, false,
			"Usage: newrecord [pathOrType] [begin] [end]\nCreates a new empty animation record\n" \
			" [pathOrType]: path to JSON, or animation type\n [begin] [end]: begin/end times\n");
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
	else
		ThrowParseException("Invalid anims command", first);
}

void CParser::ParseBaseObjectCommand(CBaseObj* pobj, const string& command, vector<string>& comps, vector<string>& args)
{
	if (command == "matrix")
	{
		NextInList(comps);
		ParseMatrixCommand(pobj->GetTM(), comps, args);
	}
	else if (command == "meta")
	{
		CheckArgs(args, 1, command, false,
			"Usage: meta [key] [value]\nReturns an existing meta value, or assigns a new one\n");

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
		CheckArgs(args, 1, command, false,
			"Usage: load [path]\nLoads a binary object\n");
		string path = ParseObjectCached<string>(args[0]);
		CBinFile binFile(CEnv::GetEnv().ExpandPathVars(path).c_str());
		if (!binFile.OpenFile(false, MigType::Object))
			ThrowParseException("Unable to open file", path);
		pobj->Load(binFile);
		binFile.CloseFile();
	}
	else if (command == "save")
	{
		CheckArgs(args, 1, command, false,
			"Usage: save [path]\nSaves the given object into a model file\n");
		string path = ParseObjectCached<string>(args[0]);
		CBinFile binFile(CEnv::GetEnv().ExpandPathVars(path).c_str());
		if (!binFile.OpenFile(true, MigType::Object))
			ThrowParseException("Unable to open file", path);
		pobj->Save(binFile);
		binFile.CloseFile();
	}
	else if (command == "name")
	{
		CheckArgs(args, 0, command, true,
			"Usage: name [newname]\nReturns the name of the object, or assigns a new name\n");
		if (m_echoActive)
		{
			cout << pobj->GetName() << "\n";
			return;
		}

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

void CParser::ParseGroupCommand(CGrp* pgrp, vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Group commands: matrix, meta, load, save, name,\n" \
				" newgroup, newpolygon, newsphere, newdirlight, newptlight, newspotlight, center, fit");
		cout << Stringify(*pgrp) << "\n";
		return;
	}

	string command = comps[0];
	if (command == "newgroup")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newgroup\nCreates a new group as a member of this group\n");
		CGrp* pnew = pgrp->CreateSubGroup();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newpolygon")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newpolygon\nCreates a new polygon as a member of this group\n");
		CPolygon* pnew = pgrp->CreatePolygon();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newsphere")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newsphere\nCreates a new sphere as a member of this group\n");
		CSphere* pnew = pgrp->CreateSphere();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newdirlight")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newdirlight\nCreates a new direction light as a member of this group\n");
		CDirLight* pnew = pgrp->CreateDirectionalLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newptlight")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newptlight\nCreates a new point light as a member of this group\n");
		CPtLight* pnew = pgrp->CreatePointLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "newspotlight")
	{
		CheckArgs(args, 0, command, false,
			"Usage: newspotlight\nCreates a new spot light as a member of this group\n");
		CSpotLight* pnew = pgrp->CreateSpotLight();
		AssignReturnValue<>(pnew);
	}
	else if (command == "center")
	{
		CheckArgs(args, 0, command, false,
			"Usage: center\nCenters this group\n");
		CBoundBox bbox;
		if (pgrp->ComputeBoundBox(bbox))
		{
			CPt center = bbox.GetCenter();
			pgrp->GetTM().Translate(-center.x, -center.y, -center.z);
		}
	}
	else if (command == "fit")
	{
		CheckArgs(args, 0, command, false,
			"Usage: fit [x] [y] [z]\nFits the group w/in x,y,z, dimensions (-1 to ignore a given dimension)\n");
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

void CParser::ParseLightCommand(CLight* plight, vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
		{
			if (plight->GetType() == ObjType::DirLight)
				throw parse_exception("Direction light commands: dir, color, highlight, shadow, matrix, meta, load, save, name");
			else if (plight->GetType() == ObjType::PtLight)
				throw parse_exception("Point light commands: origin, dropoff, color, highlight, shadow, matrix, meta, load, save, name");
			else if (plight->GetType() == ObjType::SpotLight)
				throw parse_exception("Spot light commands: origin, dir, concentration, dropoff, color, highlight, shadow, matrix, meta, load, save, name");
			else
				throw parse_exception("Unknown light type");
		}
		cout << Stringify(*plight) << "\n";
		return;
	}

	bool found = true;
	string command = comps[0];

	if (plight->GetType() == ObjType::DirLight)
	{
		CDirLight* pdirlight = (CDirLight*)plight;
		if (command == "dir")
		{
			CheckArgs(args, 1, command, true,
				"Usage: dir [dir]\nAssigns a unit vector to the direction\n");
			if (!m_echoActive)
				pdirlight->SetDirection(ParseObjectCached<CUnitVector>(args[0]));
			else
				cout << Stringify(pdirlight->GetDirection()) << "\n";
		}
		else
			found = false;
	}
	else if (plight->GetType() == ObjType::PtLight)
	{
		CPtLight* pptlight = (CPtLight*)plight;
		if (command == "origin")
		{
			CheckArgs(args, 1, command, true,
				"Usage: origin [point]\nAssigns the origin point\n");
			if (!m_echoActive)
				pptlight->SetOrigin(ParseObjectCached<CPt>(args[0]));
			else
				cout << Stringify(pptlight->GetOrigin()) << "\n";
		}
		else if (command == "dropoff")
		{
			CheckArgs(args, 2, command, true,
				"Usage: dropoff [begin] [end]\nAssigns the dropoff begin and end values\n");
			if (!m_echoActive)
				pptlight->SetDropoff(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
			else
				cout << "Begin=" << Stringify(pptlight->GetDropoffFull()) << ", end=" << Stringify(pptlight->GetDropoffDrop()) << "\n";
		}
		else
			found = false;
	}
	else if (plight->GetType() == ObjType::SpotLight)
	{
		CSpotLight* pspot = (CSpotLight*)plight;
		if (command == "origin")
		{
			CheckArgs(args, 1, command, true,
				"Usage: origin [point]\nAssigns the origin point\n");
			if (!m_echoActive)
				pspot->SetOrigin(ParseObjectCached<CPt>(args[0]));
			else
				cout << Stringify(pspot->GetOrigin()) << "\n";
		}
		else if (command == "dir")
		{
			CheckArgs(args, 1, command, true,
				"Usage: dir [dir]\nAssigns a unit vector to the direction\n");
			if (!m_echoActive)
				pspot->SetDirection(ParseObjectCached<CUnitVector>(args[0]));
			else
				cout << Stringify(pspot->GetDirection()) << "\n";
		}
		else if (command == "concentration")
		{
			CheckArgs(args, 1, command, true,
				"Usage: concentration [concentration]\nSets the spot light concentration double\n");
			if (!m_echoActive)
				pspot->SetConcentration(ParseObjectCached<double>(args[0]));
			else
				cout << Stringify(pspot->GetConcentration()) << "\n";
		}
		else if (command == "dropoff")
		{
			CheckArgs(args, 2, command, true,
				"Usage: dropoff [begin] [end]\nAssigns the dropoff begin and end values\n");
			if (!m_echoActive)
				pspot->SetDropoff(ParseObjectCached<double>(args[0]), ParseObjectCached<double>(args[1]));
			else
				cout << "Begin=" << Stringify(pspot->GetDropoffFull()) << ", end=" << Stringify(pspot->GetDropoffDrop()) << "\n";
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
			CheckArgs(args, 1, command, true,
				"Usage: color [color]\nSets the light color\n");
			if (!m_echoActive)
				plight->SetColor(ParseObjectCached<COLOR>(args[0]));
			else
				cout << Stringify(plight->GetColor()) << "\n";
		}
		else if (command == "highlight")
		{
			CheckArgs(args, 1, command, true,
				"Usage: highlight [highlight]\nSets the light highlight double\n");
			if (!m_echoActive)
				plight->SetHighlight(ParseObjectCached<double>(args[0]));
			else
				cout << plight->GetHighlight() << "\n";
		}
		else if (command == "shadow")
		{
			CheckArgs(args, 1, command, true,
				"Usage: shadow [on] [soft] [scale]\nSets the on bool, soft shadow bool, and soft shadow scale\n");
			if (!m_echoActive)
			{
				bool shadow = ParseObjectCached<bool>(args[0]);
				bool soft = (args.size() > 1 ? ParseObjectCached<bool>(args[1]) : false);
				double sscale = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : 1);
				plight->SetShadowCaster(shadow, soft, sscale);
			}
			else
				cout << "Shadow=" << plight->IsShadowCaster() << ", soft=" << plight->IsSoftShadow() << ", scale=" << plight->GetSoftScale() << "\n";
		}
		else if (command == "matrix")
		{
			NextInList(comps);
			ParseMatrixCommand(plight->GetTM(), comps, args);
		}
		else
			ParseBaseObjectCommand(plight, command, comps, args);
	}
}

void CParser::ParseObjectCommand(CObj* pobj, const string& command, vector<string>& comps, vector<string>& args)
{
	if (command == "diffuse")
	{
		CheckArgs(args, 1, command, true,
			"Usage: diffuse [color]\nSets the object diffuse color\n");
		if (!m_echoActive)
			pobj->SetDiffuse(ParseObjectCached<COLOR>(args[0]));
		else
			cout << Stringify(pobj->GetDiffuse()) << "\n";
	}
	else if (command == "specular")
	{
		CheckArgs(args, 1, command, true,
			"Usage: specular [color]\nSets the object specular color\n");
		if (!m_echoActive)
			pobj->SetSpecular(ParseObjectCached<COLOR>(args[0]));
		else
			cout << Stringify(pobj->GetSpecular()) << "\n";
	}
	else if (command == "reflection")
	{
		CheckArgs(args, 1, command, true,
			"Usage: reflection [color]\nSets the object reflection color\n");
		if (!m_echoActive)
			pobj->SetReflection(ParseObjectCached<COLOR>(args[0]));
		else
			cout << Stringify(pobj->GetReflection()) << "\n";
	}
	else if (command == "refraction")
	{
		CheckArgs(args, 1, command, true,
			"Usage: refraction [color] [index] [near] [far]\nSets the object refraction color, and optional index, near, and far planes\n");
		double index = (args.size() > 1 ? ParseObjectCached<double>(args[1]) : 1);
		double near = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : -1);
		double far = (args.size() > 3 ? ParseObjectCached<double>(args[3]) : -1);
		if (!m_echoActive)
			pobj->SetRefraction(ParseObjectCached<COLOR>(args[0]), index, near, far);
		else
			cout << Stringify(pobj->GetRefraction()) << "\n";
	}
	else if (command == "glow")
	{
		CheckArgs(args, 1, command, true,
			"Usage: glow [color]\nSets the object glow color\n");
		if (!m_echoActive)
			pobj->SetGlow(ParseObjectCached<COLOR>(args[0]));
		else
			cout << Stringify(pobj->GetGlow()) << "\n";
	}
	else if (command == "flags")
	{
		CheckArgs(args, 1, command, true,
			"Usage: flags [flags]\nSets the object flags (none, shadow, refl, refr, caster, bbox, invisible, all)\n");
		if (!m_echoActive)
			pobj->SetObjectFlags(ParseObjectFlags(args[0]));
		else
			cout << ObjectFlagsToString(pobj->GetObjectFlags()) << "\n";
	}
	else if (command == "bbox")
	{
		CheckArgs(args, 1, command, true,
			"Usage: bbox [bool]\nTurns the bounding box on/off\n");
		if (!m_echoActive)
			pobj->SetBoundBox(ParseObjectCached<bool>(args[0]));
		else
			cout << pobj->UseBoundBox() << "\n";
	}
	else if (command == "matrix")
	{
		NextInList(comps);
		ParseMatrixCommand(pobj->GetTM(), comps, args);
	}
	else if (command == "center")
	{
		CheckArgs(args, 0, command, false,
			"Usage: center\nCenters the object\n");
		CBoundBox bbox;
		pobj->ComputeBoundBox(bbox, CMatrix());
		CPt center = bbox.GetCenter();
		pobj->GetTM().Translate(-center.x, -center.y, -center.z);
	}
	else if (command == "fit")
	{
		CheckArgs(args, 0, command, false,
			"Usage: fit [x] [y] [z]\nFits the object w/in x,y,z, dimensions (-1 to ignore a given dimension)\n");
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

void CParser::ParseSphereCommand(CSphere* psphere, vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Sphere commands: origin, radius, addcolormap, addtransparencymap, addbumpmap,\n" \
				" addreflectionmap, duplicate, matrix, meta, load, save, name");
		cout << Stringify(*psphere) << "\n";
		return;
	}

	string command = comps[0];
	if (command == "origin")
	{
		CheckArgs(args, 1, command, true,
			"Usage: origin [point]\nSets the sphere origin point\n");
		if (!m_echoActive)
			psphere->SetOrigin(ParseObjectCached<CPt>(args[0]));
		else
			cout << Stringify(psphere->GetOrigin()) << "\n";
	}
	else if (command == "radius")
	{
		CheckArgs(args, 1, command, true,
			"Usage: radius [radius]\nSets the sphere radius double\n");
		if (!m_echoActive)
			psphere->SetRadius(ParseObjectCached<double>(args[0]));
		else
			cout << psphere->GetRadius() << "\n";
	}
	else if (command == "addcolormap")
	{
		CheckArgs(args, 3, command, false,
			"Usage: addcolormap [type] [flags] [title] [op] [uvmin] [uvmax]\nAdds a loaded color map by title\n" \
			" [type] can be diffuse, specular, refraction, reflection, glow, transparency, bump\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [op] can be multiply, add, subtract, blend, default multiply\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
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
		CheckArgs(args, 2, command, false,
			"Usage: addtransparencymap [flags] [title] [uvmin] [uvmax]\nAdds a transparency map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(psphere->AddTransparencyMap(flags, title, uvMin, uvMax));
	}
	else if (command == "addbumpmap")
	{
		CheckArgs(args, 2, command, false,
			"Usage: addbumpmap [flags] [title] [scale] [btol] [uvmin] [uvmax]\nAdds a bump map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [scale] is the bump scaling factor, default 1\n" \
			" [btol] is the bump tolerance int, default 0\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		double scale = (args.size() > 2 ? ParseObjectCached<double>(args[2]) : 1);
		int btol = (args.size() > 3 ? ParseObjectCached<int>(args[3]) : 0);
		UVC uvMin = (args.size() > 4 ? ParseObjectCached<UVC>(args[4]) : UVMIN);
		UVC uvMax = (args.size() > 5 ? ParseObjectCached<UVC>(args[5]) : UVMAX);
		AssignReturnValue<>(psphere->AddBumpMap(flags, title, scale, btol, uvMin, uvMax));
	}
	else if (command == "addreflectionmap")
	{
		CheckArgs(args, 2, command, false,
			"Usage: addreflectionmap [flags] [title] [uvmin] [uvmax]\nAdds a reflection map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(psphere->AddReflectionMap(flags, title, uvMin, uvMax));
	}
	else if (command == "duplicate")
	{
		CheckArgs(args, 1, command, false,
			"Usage: duplicate [other]\nDuplicate the properties of this sphere into another one\n");
		CSphere* pnew = ParseObjectCached<CSphere*>(args[0]);
		if (pnew == NULL)
			ThrowParseException("New sphere does not exist", command);
		*pnew = *psphere;
	}
	else
		ParseObjectCommand(psphere, command, comps, args);
}

void CParser::ParsePolygonCommand(CPolygon* ppoly, vector<string>& comps, vector<string>& args)
{
	if (comps.empty())
	{
		if (!m_echoActive)
			throw parse_exception("Polygon commands: loadlattice, loadlatticept, addpolycurve, addpolycurveindex,\n" \
				" addpolycurvenormal, addpolycurvemapcoord, setclockedness, extrude, addcolormap, addtransparencymap, addbumpmap,\n" \
				" addreflectionmap, applymapping, duplicate, dupfaceplate, complete, matrix, meta, load, save, name");
		cout << Stringify(*ppoly) << "\n";
		return;
	}

	string command = comps[0];
	if (command == "loadlattice")
	{
		CheckArgs(args, 1, command, false,
			"Usage: loadlattice [points]\nLoads an array of points into the lattice\n");

		CArray<CPt> lattice = ParseArray<CPt>(args[0]);
		if (lattice.GetSize() > 0)
			AssignReturnValue<>(ppoly->LoadLattice(lattice.GetSize(), lattice.ToArray()));
		else
			ThrowParseException("Invalid lattice command", args[0]);
	}
	else if (command == "loadlatticept")
	{
		CheckArgs(args, 1, command, false,
			"Usage: loadlatticept [points]\nAdds a point into the lattice\n");

		CPt pt = ParseObjectCached<CPt>(args[0]);
		AssignReturnValue<>(ppoly->LoadLatticePt(pt));
	}
	else if (command == "addpolycurve")
	{
		CheckArgs(args, 4, command, false,
			"Usage: addpolycurve [norm] [flags] [indices] [norms]\nAdds a new poly curve\n" \
			" [norm] is a unit vector normal for the entire curve\n" \
			" [flags] can be none, newplane, cull, invisible, cw, vertexnorms\n" \
			" [indices] is an array of index values\n" \
			" [norms] is an array of normals, only if vertexnorms is set\n");
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
		CheckArgs(args, 1, command, false,
			"Usage: addpolycurveindex [index]\nAdds an index to the curve index array\n");
		int index = ParseObjectCached<int>(args[0]);
		AssignReturnValue<>(ppoly->AddPolyCurveIndex(index));
	}
	else if (command == "addpolycurvenormal")
	{
		CheckArgs(args, 1, command, false,
			"Usage: addpolycurvenormal [norm]\nAdds a unit vector normal curve normals array\n");
		CUnitVector norm = ParseObjectCached<CUnitVector>(args[0]);
		AssignReturnValue<>(ppoly->AddPolyCurveNormal(norm));
	}
	else if (command == "addpolycurvemapcoord")
	{
		CheckArgs(args, 3, command, false,
			"Usage: addpolycurvemapcoord [type] [index] [uvc]\nAdds a poly curve UVC map coordinate\n" \
			" [type] can be diffuse, specular, refraction, reflection, glow, transparency, bump\n");
		TextureMapType tmap = ParseObject<TextureMapType>(args[0]);
		int index = ParseObjectCached<int>(args[1]);
		UVC uvc = ParseObjectCached<UVC>(args[2]);
		AssignReturnValue<>(ppoly->AddPolyCurveMapCoord(tmap, index, uvc));
	}
	else if (command == "setclockedness")
	{
		CheckArgs(args, 2, command, false,
			"Usage: setclockedness [ccv] [invert]\nSets polygon clockedness\n");
		AssignReturnValue<>(ppoly->SetClockedness(ParseObjectCached<bool>(args[0]), ParseObjectCached<bool>(args[1])));
	}
	else if (command == "extrude")
	{
		CheckArgs(args, 2, command, false,
			"Usage: extrude [length] [smooth] OR extrude [angles] [lengths] [smooth]\nExtudes the completed polygon\n" \
			" [length] length of single extrusion as a double\n" \
			" [angles] array of angles of extrusion as doubles\n" \
			" [lengths] array of lengths of extrusion as doubles\n" \
			" [smooth] indicates whether to smooth edges\n");
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
		CheckArgs(args, 3, command, false,
			"Usage: addcolormap [type] [flags] [title] [uvcs] [op] [uvmin] [uvmax]\nAdds a loaded color map by title\n" \
			" [type] can be diffuse, specular, refraction, reflection, glow, transparency, bump\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [uvcs] can be an optional UVC array\n" \
			" [op] can be multiply, add, subtract, blend, default multiply\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
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
		CheckArgs(args, 2, command, false,
			"Usage: addtransparencymap [flags] [title] [uvcs] [uvmin] [uvmax]\nAdds a transparency map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [uvcs] can be an optional UVC array\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		CArray<UVC> uvc = (args.size() > 2 ? ParseArray<UVC>(args[2]) : CArray<UVC>());
		UVC uvMin = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMIN);
		UVC uvMax = (args.size() > 4 ? ParseObjectCached<UVC>(args[4]) : UVMAX);
		AssignReturnValue<>(ppoly->AddTransparencyMap(flags, title, uvc.ToArray(), uvc.GetSize(), uvMin, uvMax));
	}
	else if (command == "addbumpmap")
	{
		CheckArgs(args, 2, command, false,
			"Usage: addbumpmap [flags] [title] [scale] [btol] [uvcs] [uvmin] [uvmax]\nAdds a bump map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [scale] is the bump scaling factor, default 1\n" \
			" [btol] is the bump tolerance int, default 0\n" \
			" [uvcs] can be an optional UVC array\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
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
		CheckArgs(args, 2, command, false,
			"Usage: addreflectionmap [flags] [title] [uvmin] [uvmax]\nAdds a reflection map by title\n" \
			" [flags] can be none, enabled, rgb, alpha, invert, bump, invertalpha\n" \
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
		dword flags = ParseTexturingFlags(args[0]);
		string title = ParseObjectCached<string>(args[1]);
		UVC uvMin = (args.size() > 2 ? ParseObjectCached<UVC>(args[2]) : UVMIN);
		UVC uvMax = (args.size() > 3 ? ParseObjectCached<UVC>(args[3]) : UVMAX);
		AssignReturnValue<>(ppoly->AddReflectionMap(flags, title, uvMin, uvMax));
	}
	else if (command == "applymapping")
	{
		CheckArgs(args, 4, command, false,
			"Usage: applymapping [wtype] [type] [mapping] [indc] [center] [axis] [uvmin] [uvmax]\nApplies a mapping to an existing texture\n" \
			" [wtype] can be spherical, elliptical, projection, extrusion\n" \
			" [type] can be diffuse, specular, refraction, reflection, glow, transparency, bump\n" \
			" [mapping] map index\n"
			" [indc] use individual index coordinates\n"
			" [center] optional center point\n"
			" [axis] optional axis unit vector\n"
			" [uvmin] [uvmax] are UV coordinates, defaults (0, 0) and (1, 1)\n");
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
		CheckArgs(args, 1, command, false,
			"Usage: duplicate [other]\nDuplicate the properties of this polygon into another one\n");
		CPolygon* pnew = ParseObjectCached<CPolygon*>(args[0]);
		if (pnew == NULL)
			ThrowParseException("New polygon does not exist", command);
		*pnew = *ppoly;
	}
	else if (command == "dupfaceplate")
	{
		CheckArgs(args, 2, command, false,
			"Usage: dupfaceplate [newpoly] [hideoriginal]\nDuplicates a face plate into a new polygon\n" \
			" [newpoly] name of existing polygon to place the new face plate\n" \
			" [hideoriginal] indicates original face plate should be hidden\n");
		CPolygon* pnew = ParseObjectCached<CPolygon*>(args[0]);
		bool hideOriginalFacePlate = ParseObjectCached<bool>(args[1]);
		if (pnew == NULL)
			ThrowParseException("New polygon does not exist", command);
		ppoly->DupExtrudedFacePlate(pnew, hideOriginalFacePlate);
	}
	else if (command == "complete")
	{
		CheckArgs(args, 0, command, false,
			"Usage: complete\nIndicates that the polygon lattice is complete loading\n");
		AssignReturnValue<>(ppoly->LoadComplete());
	}
	else
		ParseObjectCommand(ppoly, command, comps, args);
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
				CParser parser(&model);
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

bool CParser::ParseCommandStringInner(const string& line)
{
	m_helpActive = false;
	m_echoActive = false;

	vector<string> args;
	if (SplitRootString(line, args) == 0)
		return true;

	string root = NextInList(args);

	try
	{
		if (root == "help")
		{
			// help command
			if (args.empty())
			{
				throw parse_exception(
					"Welcome to the MigRender parser help\n" \
					" Enter \"help <cmd>\" to get context sensitive help\n" \
					" Enter \"echo <cmd>\" to echo certain values to the console\n" \
					" Enter \"var <variable>:<type>\" to create a new variable, or to assign a variable to a returned value\n" \
					" Use '.' as a shortcut for the super group, and then grp?/obj?/lit? to drill into group members\n" \
					" Commands: sampling, renderFlags, ambient, fade, fog, load, save, loadpackage, loadtext, script, return, quit,\n" \
					"  camera, bg, images, anims, <primitive>, <object>\n");
			}
			m_helpActive = true;

			// continue processing, so update the root
			root = NextInList(args);
		}
		else if (root == "echo")
		{
			if (args.empty())
				throw parse_exception("Use 'echo <cmd>' to echo certain values to the console\n");
			m_echoActive = true;

			// continue processing, so update the root
			root = NextInList(args);
		}

		bool bContinue = true;
		if (root == "var")
		{
			// create variable command
			CheckArgs(args, 1, root, false,
				"Usage: var [name]:[type] = [value]\nCreates a named variable w/ optional type, and optionally sets it's initial value\n");
			if (!ParseCreateCommand(args))
			{
				// continue processing, so update the root
				root = NextInList(args);
			}
			else
				bContinue = false;
		}

		if (bContinue)
		{
			// global model commands
			if (root == "sampling")
			{
				CheckArgs(args, 1, root, true,
					"Usage: sampling [mode]\nChanges super sampling\n [mode]: 1x, 5x, 9x, edge, object\n");
				if (!m_echoActive)
					m_pmodel->SetSampling(ParseObject<SuperSample>(args[0]));
				else
					cout << EnumToString(m_pmodel->GetSampling()) << "\n";
			}
			else if (root == "renderflags")
			{
				CheckArgs(args, 1, root, true,
					"Usage: renderflags [quality]\nChanges rendering flags\n [quality]: none, reflect, refract, shadows, softshadows, all\n");
				if (!m_echoActive)
					m_pmodel->SetRenderQuality(ParseRenderFlags(args[0]));
				else
					cout << RenderFlagsToString(m_pmodel->GetRenderQuality()) << "\n";
			}
			else if (root == "ambient")
			{
				CheckArgs(args, 1, root, true,
					"Usage: ambient [color]\nChanges ambient color\n");
				if (!m_echoActive)
					m_pmodel->SetAmbientLight(ParseObjectCached<COLOR>(args[0]));
				else
					cout << Stringify(m_pmodel->GetAmbientLight()) << "\n";
			}
			else if (root == "fade")
			{
				CheckArgs(args, 1, root, true,
					"Usage: fade [color]\nChanges fade color\n");
				if (!m_echoActive)
					m_pmodel->SetFade(ParseObjectCached<COLOR>(args[0]));
				else
					cout << Stringify(m_pmodel->GetFade()) << "\n";
			}
			else if (root == "fog")
			{
				CheckArgs(args, 3, root, true,
					"Usage: fog [color] [near] [far]\nChanges fog settings\n [color]: fog color\n [near]: near distance\n [far]: far distance\n");
				if (!m_echoActive)
				{
					m_pmodel->SetFog(
						ParseObjectCached<COLOR>(args[0]),
						ParseObjectCached<double>(args[1]),
						ParseObjectCached<double>(args[2]));
				}
				else
				{
					cout << Stringify(m_pmodel->GetFog()) 
						<< ", near=" << to_string(m_pmodel->GetFogNear())
						<< ", far=" + to_string(m_pmodel->GetFogFar()) << "\n";
				}
			}
			else if (root == "load")
			{
				CheckArgs(args, 1, root, false,
					"Usage: load [path]\nLoads a model\n");
				string path = ParseObjectCached<string>(args[0]);
				CBinFile binFile(CEnv::GetEnv().ExpandPathVars(path).c_str());
				if (!binFile.OpenFile(false, MigType::Model))
					ThrowParseException("Unable to open file", path);
				m_pmodel->Load(binFile);
				binFile.CloseFile();
			}
			else if (root == "save")
			{
				CheckArgs(args, 1, root, false,
					"Usage: save [path]\nSaves the current model\n");
				string path = ParseObjectCached<string>(args[0]);
				CBinFile binFile(CEnv::GetEnv().ExpandPathVars(path).c_str());
				if (!binFile.OpenFile(true, MigType::Model))
					ThrowParseException("Unable to open file", path);
				m_pmodel->Save(binFile);
				binFile.CloseFile();
			}
			else if (root == "loadpackage")
			{
				CheckArgs(args, 2, root, false,
					"Usage: loadpackage [path] [name]\nLoads a model from a package\n [path]: path to package\n [name]: name of model in the package\n");
				string path = ParseObjectCached<string>(args[0]);
				string name = ParseObjectCached<string>(args[1]);
				CGrp* pgrp = (args.size() > 2 ? ParseObjectCached<CGrp*>(args[2]) : &m_pmodel->GetSuperGroup());

				CBinFile binFile(CEnv::GetEnv().ExpandPathVars(path).c_str());
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
			else if (root == "loadtext")
			{
				CheckArgs(args, 3, root, false,
					"Usage: loadtext [package] [text] [group] [script] [suffix]\nLoads a text string from a text package\n" \
					" [package]: path to package file\n [text]: text string\n [group]: name of existing group to load into\n" \
					" [script]: path to optional script executed for each letter\n [suffix]: suffix for each letter\n");
				string pkg = ParseObjectCached<string>(args[0]);
				string text = ParseObjectCached<string>(args[1]);
				CGrp* pgrp = ParseObjectCached<CGrp*>(args[2]);
				string script = (args.size() > 3 ? ParseObjectCached<string>(args[3]) : "");
				string suffix = (args.size() > 4 ? ParseObjectCached<string>(args[4]) : "");

				CBinFile binFile(CEnv::GetEnv().ExpandPathVars(pkg).c_str());
				CPackage package;
				if (package.OpenPackage(&binFile) == 0)
					ThrowParseException("Unable to open package", pkg);
				AssignReturnValue<>(LoadTextString(package, *m_pmodel, pgrp, text, suffix, script, m_envpath));
			}
			else if (root == "script")
			{
				CheckArgs(args, 1, root, false,
					"Usage: script [path]\nExecutes a script file\n");
				ParseScriptCommand(args);
			}
			else if (root == "return")
			{
				CheckArgs(args, 1, root, false,
					"Usage: return [var name]\nReturns an existing variable to the calling script\n");
				m_scriptReturnVarName = args[0];
			}
			else
			{
				// split the root into it's nexted components (ie. "object.command")
				vector<string> comp;
				if (SplitNestedComponents(root, comp) > 0)
				{
					string rootComponent = NextInList(comp);

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
									string rootComponent = NextInList(comp);

									pobj = DrillDown((CGrp*)pobj, rootComponent, comp);
								}
							}
							if (pobj == NULL)
								ThrowParseException("Bad root component", rootComponent);

							if (comp.size() > 0 || m_helpActive || m_echoActive || m_returnVarName.empty())
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
			{
				m_returnVarName.clear();
				ThrowParseException("Unable to assign return var, command did not return a value", root);
			}
		}
	}
	catch (const parse_exception&)
	{
		// re-throw
		throw;
	}
	catch (const mig_exception& e)
	{
		// convert to a parse_exception
		throw parse_exception(e.what(), "", m_script, m_line);
	}

	return true;
}

template <typename T>
bool CParser::SetEnvVar(const string& name, const T& value)
{
	map<string, T>& mapVars = GetVarMap<T>();
	mapVars[name] = value;

	return true;
}

template <typename T>
bool CParser::GetEnvVar(const string& name, T& value)
{
	map<string, T>& mapVars = GetVarMap<T>();
	value = mapVars[name];

	return true;
}

template bool CParser::SetEnvVar(const string& name, const int& value);
template bool CParser::GetEnvVar(const string& name, int& value);
template bool CParser::SetEnvVar(const string& name, const double& value);
template bool CParser::GetEnvVar(const string& name, double& value);
template bool CParser::SetEnvVar(const string& name, const bool& value);
template bool CParser::GetEnvVar(const string& name, bool& value);
template bool CParser::SetEnvVar(const string& name, const string& value);
template bool CParser::GetEnvVar(const string& name, string& value);
template bool CParser::SetEnvVar(const string& name, const COLOR& value);
template bool CParser::GetEnvVar(const string& name, COLOR& value);
template bool CParser::SetEnvVar(const string& name, const UVC& value);
template bool CParser::GetEnvVar(const string& name, UVC& value);
template bool CParser::SetEnvVar(const string& name, const CPt& value);
template bool CParser::GetEnvVar(const string& name, CPt& value);
template bool CParser::SetEnvVar(const string& name, const CMatrix& value);
template bool CParser::GetEnvVar(const string& name, CMatrix& value);
template bool CParser::SetEnvVar(const string& name, const CUnitVector& value);
template bool CParser::GetEnvVar(const string& name, CUnitVector& value);

template <typename T>
bool CParser::SetEnvVarModelRef(const string& name, T* value)
{
	map<string, T*>& mapVars = GetVarMap<T*>();
	mapVars[name] = value;

	return true;
}

template <typename T>
bool CParser::GetEnvVarModelRef(const string& name, T*& value)
{
	map<string, T*>& mapVars = GetVarMap<T*>();
	value = mapVars[name];

	return true;
}

template bool CParser::SetEnvVarModelRef(const string& name, CGrp* value);
template bool CParser::GetEnvVarModelRef(const string& name, CGrp*& value);
template bool CParser::SetEnvVarModelRef(const string& name, CSphere* value);
template bool CParser::GetEnvVarModelRef(const string& name, CSphere*& value);
template bool CParser::SetEnvVarModelRef(const string& name, CPolygon* value);
template bool CParser::GetEnvVarModelRef(const string& name, CPolygon*& value);
template bool CParser::SetEnvVarModelRef(const string& name, CDirLight* value);
template bool CParser::GetEnvVarModelRef(const string& name, CDirLight*& value);
template bool CParser::SetEnvVarModelRef(const string& name, CPtLight* value);
template bool CParser::GetEnvVarModelRef(const string& name, CPtLight*& value);
template bool CParser::SetEnvVarModelRef(const string& name, CSpotLight* value);
template bool CParser::GetEnvVarModelRef(const string& name, CSpotLight*& value);

bool CParser::ParseCommandString(const string& line)
{
	m_line = 0;

	if (m_pmodel == NULL)
		throw parse_exception("Model not set, cannot parse");

	return ParseCommandStringInner(line);
}

bool CParser::ParseCommandScript(istream& stream)
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
	return true;
}

bool CParser::ParseCommandScript(const string& path)
{
	if (m_pmodel == NULL)
		throw parse_exception("Model not set, cannot parse");

	string expandedPath = CEnv::GetEnv().ExpandPathVars(path);

	fstream infile;
	infile.open(path, ios_base::in);
	if (!infile.is_open())
	{
		infile.open(m_envpath + expandedPath, ios_base::in);
		if (!infile.is_open())
			throw parse_exception("Failed to open script file: " + path);
	}
	m_script = expandedPath;

	size_t endOfPath = expandedPath.rfind('\\');
	if (endOfPath == string::npos)
		endOfPath = expandedPath.rfind('/');
	if (endOfPath != string::npos)
		m_envpath = expandedPath.substr(0, endOfPath + 1);

	bool ret = ParseCommandScript(infile);
	infile.close();
	return ret;
}

bool CParser::ParseCommandScriptString(const string& script)
{
	if (m_pmodel == NULL)
		throw parse_exception("Model not set, cannot parse");

	stringstream strstream(script);
	m_script = "<embedded>";
	return ParseCommandScript(strstream);
}
