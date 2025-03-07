#include "StdAfx.h"
#include "Reg.h"

using namespace MigRender;

static HKEY g_hKey = NULL;
static TCHAR g_script[MAX_PATH];

BOOL InitRegistry(void)
{
	LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\MigRender\\TestApp"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &g_hKey, NULL);
	return (ret == ERROR_SUCCESS);
}

DWORD GetRegistryInt(LPCTSTR entry)
{
	if (g_hKey)
	{
		DWORD value;
		DWORD cnt = sizeof(DWORD);
		LONG ret = RegQueryValueEx(g_hKey, entry, 0, NULL, (LPBYTE) &value, &cnt);
		if (ret == ERROR_SUCCESS)
			return value;
	}
	return 0;
}

BOOL SetRegistryInt(LPCTSTR entry, DWORD value)
{
	if (g_hKey)
	{
		LONG ret = RegSetValueEx(g_hKey, entry, 0, REG_DWORD, (const BYTE*) &value, sizeof(DWORD));
		return (ret == ERROR_SUCCESS);
	}
	return FALSE;
}

BOOL GetRegistryStr(LPCTSTR entry, LPTSTR value, DWORD cnt)
{
	if (g_hKey)
	{
		LONG ret = RegQueryValueEx(g_hKey, entry, 0, NULL, (LPBYTE) value, &cnt);
		return (ret == ERROR_SUCCESS);
	}
	return FALSE;
}

BOOL SetRegistryStr(LPCTSTR entry, LPCTSTR value, DWORD cnt)
{
	if (g_hKey)
	{
		LONG ret = RegSetValueEx(g_hKey, entry, 0, REG_SZ, (const BYTE*) value, 2*cnt);
		return (ret == ERROR_SUCCESS);
	}
	return FALSE;
}

void SetScriptFile(LPCTSTR script)
{
	if (script)
		_tcscpy_s(g_script, MAX_PATH, script);
	else
		g_script[0] = 0;
}

UINT GetScriptInt(LPCTSTR section, LPCTSTR key, int def)
{
	return GetPrivateProfileInt(section, key, def, g_script);
}

double GetScriptFloat(LPCTSTR section, LPCTSTR key, double def)
{
	double ret = def;

	char tmp[20];
	if (GetScriptString(section, key, _T("0"), tmp, 20))
	{
		ret = atof(tmp);
	}
	return ret;
}

DWORD GetScriptString(LPCTSTR section, LPCTSTR key, LPCTSTR def, char *ret, DWORD size)
{
	TCHAR tmp[MAX_PATH];
	if (GetPrivateProfileString(section, key, def, tmp, MAX_PATH, g_script))
	{
		return WideCharToMultiByte(CP_ACP, 0, tmp, -1, ret, size, NULL, NULL);
	}

	return 0;
}

COLOR GetScriptColor(LPCTSTR section, LPCTSTR key, const COLOR& def)
{
	COLOR ret = def;

	char color[80], *context;
	if (GetScriptString(section, key, _T("0,0,0"), color, 80))
	{
		ret.r = atof(strtok_s(color, ",", &context));
		ret.g = atof(strtok_s(NULL, ",", &context));
		ret.b = atof(strtok_s(NULL, ",", &context));
		ret.a = 0;
	}
	return ret;
}

CPt GetScriptPt(LPCTSTR section, LPCTSTR key, const CPt& def)
{
	CPt ret = def;

	char point[80], *context;
	if (GetScriptString(section, key, _T("0,0,0"), point, 80))
	{
		ret.x = atof(strtok_s(point, ",", &context));
		ret.y = atof(strtok_s(NULL, ",", &context));
		ret.z = atof(strtok_s(NULL, ",", &context));
	}
	return ret;
}

void CloseRegistry(void)
{
	if (g_hKey)
		RegCloseKey(g_hKey);
	g_hKey = NULL;
}
