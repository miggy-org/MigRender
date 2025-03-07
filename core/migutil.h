#pragma once

#include <string>
#include <map>

#include "defines.h"

using namespace std;

_MIGRENDER_BEGIN

class CEnv
{
private:
	map<string, string> _vars;

public:
	static CEnv& GetEnv();

	void SetHomePath(const string& home);
	void AddPathVar(const string& name, const string& val);

	string ExpandPathVars(const string& path) const;
};

_MIGRENDER_END
