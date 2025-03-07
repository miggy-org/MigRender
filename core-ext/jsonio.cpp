// jsonio.cpp - defines JSON I/O utility classes
//

#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "../core/anim.h"
#include "jsonio.h"
#include "parser.h"

using namespace std;
using namespace MigRender;

static const string KeywordClass = "class";
static const string KeywordType = "type";
static const string KeywordInterpolation = "interpolation";
static const string KeywordOperation = "operation";
static const string KeywordBias = "bias";
static const string KeywordBias2 = "bias2";
static const string KeywordApplyToStart = "applyToStart";
static const string KeywordReverse = "reverse";
static const string KeywordBeginTime = "beginTime";
static const string KeywordEndTime = "endTime";
static const string KeywordBeginValue = "beginValue";
static const string KeywordEndValue = "endValue";

static const string JsonClassCPt = "CPt";
static const string JsonClassCOLOR = "COLOR";

static const string JsonMemberCPtX = "x";
static const string JsonMemberCPtY = "y";
static const string JsonMemberCPtZ = "z";
static const string JsonMemberCOLORR = "r";
static const string JsonMemberCOLORG = "g";
static const string JsonMemberCOLORB = "b";
static const string JsonMemberCOLORA = "a";

template <typename T>
static T ParseJsonObject(const Json::Value& root)
{
	throw parse_exception("Unsupported JSON object type");
}

template <>
static CPt ParseJsonObject(const Json::Value& root)
{
	if (root[KeywordClass].asString() != JsonClassCPt)
		throw parse_exception("Incorrect class label: " + root[KeywordClass].asString());
	return CPt(root[JsonMemberCPtX].asDouble(), root[JsonMemberCPtY].asDouble(), root[JsonMemberCPtZ].asDouble());
}

template <>
static COLOR ParseJsonObject(const Json::Value& root)
{
	if (root[KeywordClass].asString() != JsonClassCOLOR)
		throw parse_exception("Incorrect class label: " + root[KeywordClass].asString());
	return COLOR(root[JsonMemberCOLORR].asDouble(), root[JsonMemberCOLORG].asDouble(), root[JsonMemberCOLORB].asDouble(), root[JsonMemberCOLORA].asDouble());
}

template <>
static CAnimRecord ParseJsonObject(const Json::Value& root)
{
	CAnimRecord record;
	if (!root[KeywordType].isNull())
		record.SetAnimType(CParser::ParseEnum<AnimType>(root[KeywordType].asString()));
	if (!root[KeywordInterpolation].isNull())
		record.SetInterpolation(CParser::ParseEnum<AnimInterpolation>(root[KeywordInterpolation].asString()));
	if (!root[KeywordOperation].isNull())
		record.SetOperation(CParser::ParseEnum<AnimOperation>(root[KeywordOperation].asString()));
	if (!root[KeywordBias].isNull())
		record.SetBias(root[KeywordBias].asDouble());
	if (!root[KeywordBias2].isNull())
		record.SetBias2(root[KeywordBias2].asDouble());
	if (!root[KeywordApplyToStart].isNull())
		record.SetApplyToStart(root[KeywordApplyToStart].asBool());
	if (!root[KeywordReverse].isNull())
		record.SetReverse(root[KeywordReverse].asBool());

	double beginTime = (!root[KeywordBeginTime].isNull() ? root[KeywordBeginTime].asDouble() : record.GetBeginTime());
	double endTime = (!root[KeywordEndTime].isNull() ? root[KeywordEndTime].asDouble() : record.GetEndTime());
	record.SetTimes(beginTime, endTime);

	Json::ValueType type = root[KeywordBeginValue].type();
	switch (type)
	{
	case Json::ValueType::intValue:
	case Json::ValueType::realValue:
		record.SetValues(root[KeywordBeginValue].asDouble(), root[KeywordEndValue].asDouble());
		break;
	case Json::ValueType::objectValue:
	{
		Json::Value beginValue = root[KeywordBeginValue];
		Json::Value endValue = root[KeywordEndValue];

		if (beginValue[KeywordClass].asString() == JsonClassCPt)
			record.SetValues(ParseJsonObject<CPt>(beginValue), ParseJsonObject<CPt>(endValue));
		else if (beginValue[KeywordClass].asString() == JsonClassCOLOR)
			record.SetValues(ParseJsonObject<COLOR>(beginValue), ParseJsonObject<COLOR>(endValue));
	}
	break;
	}

	return record;
}

template <typename T>
T JsonReader::LoadFromJsonFile(Json::Value& root)
{
	throw parse_exception("Unknown type, unable to load from json");
}

template <>
CAnimRecord JsonReader::LoadFromJsonFile(Json::Value& root)
{
	return ParseJsonObject<CAnimRecord>(root);
}

template <typename T>
T JsonReader::LoadFromJsonFile(std::ifstream& ifs, const string& path)
{
	Json::CharReaderBuilder builder;
	Json::Value root;
	JSONCPP_STRING errs;
	if (!parseFromStream(builder, ifs, &root, &errs))
		throw parse_exception("Unable to parse json stream: " + path);

	return LoadFromJsonFile<T>(root);
}

template <typename T>
T JsonReader::LoadFromJsonFile(const std::string& path, const std::string& envpath)
{
	std::ifstream ifs;
	ifs.open(path);
	if (!ifs.is_open() && !envpath.empty())
		ifs.open(envpath + path, ios_base::in);

	return LoadFromJsonFile<T>(ifs, path);
}

template CAnimRecord JsonReader::LoadFromJsonFile(const std::string& path, const std::string& envpath);
