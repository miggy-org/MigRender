#include "migutil.h"

#include <regex>
#include <sys/stat.h>

using namespace MigRender;

CEnv _singleton;

CEnv& CEnv::GetEnv()
{
	return _singleton;
}

void CEnv::SetHomePath(const string& home)
{
	string lhome = home;
	size_t last_slash_pos = lhome.find('\\');
	if (last_slash_pos != lhome.length() - 1)
		lhome.append("\\");

	AddPathVar("$IMAGES", lhome + "images");
	AddPathVar("$MODELS", lhome + "models");
	AddPathVar("$MOVIES", lhome + "movies");
	AddPathVar("$SCRIPTS", lhome + "scripts");
}

void CEnv::AddPathVar(const string& name, const string& val)
{
	if (!name.empty() && !val.empty())
	{
		struct stat buffer;
		if (stat(val.c_str(), &buffer) == 0)
			_vars[name] = val;
	}
}

string CEnv::ExpandPathVars(const string& path) const
{
	string out = path;
	while (out.find('$') != string::npos)
	{
		bool replaced = false;

		for (const auto& key : _vars)
		{
			size_t start_pos = out.find(key.first);
			if (start_pos != string::npos)
			{
				out.replace(start_pos, key.first.length(), key.second);
				replaced = true;
			}
		}

		if (!replaced)
			break;
	}

	return out;
}
