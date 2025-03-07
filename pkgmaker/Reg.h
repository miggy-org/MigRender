#pragma once

#include "vector.h"

BOOL InitRegistry(void);

DWORD GetRegistryInt(LPCTSTR entry, DWORD defvalue);
BOOL GetRegistryStr(LPCTSTR entry, LPTSTR value, DWORD cnt);
BOOL SetRegistryInt(LPCTSTR entry, DWORD value);
BOOL SetRegistryStr(LPCTSTR entry, LPCTSTR value, DWORD cnt);

void CloseRegistry(void);
