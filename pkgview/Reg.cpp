#include "StdAfx.h"
#include "Reg.h"

static HKEY g_hKey = NULL;

BOOL InitRegistry(void)
{
	LONG ret = RegCreateKeyEx(HKEY_CURRENT_USER, _T("Software\\MigRender\\PackageViewer"), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &g_hKey, NULL);
	return (ret == ERROR_SUCCESS);
}

DWORD GetRegistryInt(LPCTSTR entry, DWORD defvalue)
{
	if (g_hKey)
	{
		DWORD value;
		DWORD cnt = sizeof(DWORD);
		LONG ret = RegQueryValueEx(g_hKey, entry, 0, NULL, (LPBYTE) &value, &cnt);
		if (ret == ERROR_SUCCESS)
			return value;
	}
	return defvalue;
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

void CloseRegistry(void)
{
	if (g_hKey)
		RegCloseKey(g_hKey);
	g_hKey = NULL;
}
