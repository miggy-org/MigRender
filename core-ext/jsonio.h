// jsonio.h - defines JSON I/O utility classes
//

#pragma once

#include <string>

#include "../lib/jsoncpp/json.h"

#include "../core/defines.h"

_MIGRENDER_BEGIN

class JsonReader
{
public:
	template <typename T>
	static T LoadFromJsonFile(Json::Value& root);
	template <typename T>
	static T LoadFromJsonFile(std::ifstream& ifs, const std::string& path);
	template <typename T>
	static T LoadFromJsonFile(const std::string& path, const std::string& envpath);
};

_MIGRENDER_END
