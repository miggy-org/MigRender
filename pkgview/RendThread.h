#pragma once
#include "stdafx.h"
#include "rendbitmap.h"

#include <string>
#include <vector>
using namespace std;

#define WM_REND_START		WM_USER + 1
#define WM_REND_FINISH		WM_USER + 2

struct REND_THREAD_DATA
{
	HWND hdlg;
	CRendBitmap* pbmp;

	char szPackage[MAX_PATH];
	char szScript[MAX_PATH];
	char szText[80];
	char szLookupSuffix[20];

	char szBackdrop[MAX_PATH];
	char szLights[MAX_PATH];

	char szTexture1[MAX_PATH];
	char szTexture2[MAX_PATH];
	char szReflect[MAX_PATH];

	char szBackdropTexture1[MAX_PATH];
	char szBackdropTexture2[MAX_PATH];
	char szBackdropReflect[MAX_PATH];
	char szUserImage[MAX_PATH];
};

struct BATCH_THREAD_DATA
{
	HWND hdlg;
	CRendBitmap* pbmp;

	vector<string> packages;
	vector<string> prefixes;
	vector<string> suffixes;
	vector<string> fontScripts;
	vector<string> setupScripts;

	int width;
	int height;
	string dest;

	char szTexture1[MAX_PATH];
	char szTexture2[MAX_PATH];
	char szReflect[MAX_PATH];
};

extern DWORD WINAPI RendThreadProc(LPVOID lpParam);
extern DWORD WINAPI BatchThreadProc(LPVOID lpParam);
