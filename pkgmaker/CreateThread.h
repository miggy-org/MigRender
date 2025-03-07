#pragma once
#include "stdafx.h"

#include <string>
#include <vector>
using namespace std;

#define WM_CREATE_START		WM_USER + 1
#define WM_CREATE_FINISH	WM_USER + 2
#define WM_CREATE_UPDATE	WM_USER + 3

struct CREATE_FONT_DATA
{
	HWND hdlg;

	TCHAR szFont[80];
	TCHAR szScript[MAX_PATH];
	TCHAR szOutput[MAX_PATH];
	int xtxtdiv, ytxtdiv;

	// font attributes
	int nWeight;
	BOOL bItalic;
	BOOL bUnderline;
	BOOL bComplete;
};

struct CREATE_ASSET_DATA
{
	HWND hdlg;

	TCHAR szScript[MAX_PATH];
};

struct FONT_SETTING
{
	string font;
	string filename;
	int xdiv;
	int ydiv;
	bool bold;
	bool italic;
	bool underline;
	bool complete;
};

struct CREATE_BATCH_DATA
{
	HWND hdlg;

	vector<FONT_SETTING> fontSettings;
	vector<string> fontScripts;
	vector<string> assetScripts;
};

extern DWORD WINAPI CreateFontProc(LPVOID lpParam);
extern DWORD WINAPI CreateAssetProc(LPVOID lpParam);
extern DWORD WINAPI CreateBatchProc(LPVOID lpParam);
