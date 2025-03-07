#pragma once

#include "vector.h"

BOOL InitRegistry(void);

DWORD GetRegistryInt(LPCTSTR entry);
BOOL GetRegistryStr(LPCTSTR entry, LPTSTR value, DWORD cnt);
BOOL SetRegistryInt(LPCTSTR entry, DWORD value);
BOOL SetRegistryStr(LPCTSTR entry, LPCTSTR value, DWORD cnt);

void SetScriptFile(LPCTSTR script);
UINT GetScriptInt(LPCTSTR section, LPCTSTR key, int def);
double GetScriptFloat(LPCTSTR section, LPCTSTR key, double def);
DWORD GetScriptString(LPCTSTR section, LPCTSTR key, LPCTSTR def, char *ret, DWORD size);
MigRender::COLOR GetScriptColor(LPCTSTR section, LPCTSTR key, const MigRender::COLOR& def);
MigRender::CPt GetScriptPt(LPCTSTR section, LPCTSTR key, const MigRender::CPt& def);

void CloseRegistry(void);
