// TestApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <commctrl.h>

#include <fstream>
using namespace std;

#include "PackageMaker.h"
#include "Reg.h"

#include "createthread.h"

#define MAX_LOADSTRING	100
const TCHAR _szDefaultFontPkg[] = _T("..\\models\\font.pkg");

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
DWORD threadID = 0;

// Fonts
TCHAR szChosenFont[80] = {};
TCHAR szFontScript[MAX_PATH] = {};
int xdiv = 1;
int ydiv = 1;
BOOL bComplete = TRUE;
BOOL bBold = FALSE;
BOOL bItalic = FALSE;
BOOL bUnderline = FALSE;  // doesn't seem to work

// Assets
TCHAR szAssetScript[MAX_PATH] = {};

static void CenterWindow(HWND hWnd)
{
	RECT rWnd;
	GetClientRect(hWnd, &rWnd);
	int width = rWnd.right;
	int height = rWnd.bottom;

	int screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
	int screen_height = GetSystemMetrics(SM_CYFULLSCREEN);
	int x = (screen_width - width) / 2;
	int y = (screen_height - height) / 2;

	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

static TCHAR* GetTitle(TCHAR* szPath)
{
	TCHAR* p = _tcsrchr(szPath, _T('\\'));
	if (p != NULL)
		return ++p;
	return szPath;
}

static TCHAR* GetPath(const TCHAR* szPath)
{
	static TCHAR szBuffer[MAX_PATH] = {};

	_tcscpy_s(szBuffer, szPath);
	TCHAR* p = _tcsrchr(szBuffer, _T('\\'));
	if (p != NULL)
	{
		*(++p) = 0;
		return szBuffer;
	}
	return NULL;
}

///////////////////////////////////////////////////////////
// Font picker dialog

int CALLBACK EnumTTFonts(ENUMLOGFONTEX* lpelfe, NEWTEXTMETRICEX* lpntme, DWORD dwFontType, LPARAM param)
{
	if (dwFontType == TRUETYPE_FONTTYPE)
	{
		HWND hlb = (HWND) param;
		SendMessage(hlb, LB_ADDSTRING, 0, (LPARAM) lpelfe->elfFullName);
	}

	return 1;
}

static void UpdateTextureTracker(HWND hDlg)
{
	HWND hslider = GetDlgItem(hDlg, IDC_TEXTURE);
	//int pos = HIWORD(wParam);
	int pos = SendMessage(hslider, TBM_GETPOS, 0, 0);
	HWND hstatus = GetDlgItem(hDlg, IDC_STATUS);
	TCHAR szStatus[8];
	switch (pos)
	{
	default: wcscpy_s(szStatus, 8, _T("1")); break;
	case 2: wcscpy_s(szStatus, 8, _T("4")); break;
	case 3: wcscpy_s(szStatus, 8, _T("9")); break;
	case 4: wcscpy_s(szStatus, 8, _T("16")); break;
	}
	SetWindowText(hstatus, szStatus);
}

static void OnInitFontDialog(HWND hdlg)
{
	HWND hlb = GetDlgItem(hdlg, IDC_FONTS);
	if (hlb)
	{
		LOGFONT lf;
		memset(&lf, 0, sizeof(LOGFONT));
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfFaceName[0] = '\0';

		HDC hDC = GetDC(NULL);
		EnumFontFamiliesEx(hDC, &lf, (FONTENUMPROC) EnumTTFonts, (LPARAM) hlb, 0);
	}

	HWND hslider = GetDlgItem(hdlg, IDC_TEXTURE);
	if (hslider)
	{
		SendMessage(hslider, TBM_SETRANGE, TRUE, MAKELONG(1, 4));
		SendMessage(hslider, TBM_SETPOS, TRUE, 3);
	}
	UpdateTextureTracker(hdlg);

	CheckDlgButton(hdlg, IDC_COMPLETE, bComplete);
	CheckDlgButton(hdlg, IDC_BOLD, bBold);
	CheckDlgButton(hdlg, IDC_ITALIC, bItalic);
	//CheckDlgButton(hdlg, IDC_UNDERLINE, bUnderline);
	PostMessage(hdlg, WM_COMMAND, MAKEWPARAM(IDC_COMPLETE, BN_CLICKED), 0);
}

static void OnFontScript(HWND hDlg)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Script Files (*.txt)\0*.txt\0\0");
	ofn.lpstrTitle = _T("Script Chooser");
	ofn.lpstrFile = szFontScript;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		SetRegistryStr(_T("fontscript"), szFontScript, _tcslen(szFontScript) + 1);
	}
}

INT_PTR CALLBACK FontProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnInitFontDialog(hDlg);
		return (INT_PTR)TRUE;

	case WM_HSCROLL:
		//if (LOWORD(wParam) == TB_THUMBTRACK || LOWORD(wParam) == TB_THUMBPOSITION)
		{
			UpdateTextureTracker(hDlg);
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				if (LOWORD(wParam) == IDOK)
				{
					HWND hlb = GetDlgItem(hDlg, IDC_FONTS);
					int index = (int) SendMessage(hlb, LB_GETCURSEL, 0, 0);
					SendMessage(hlb, LB_GETTEXT, index, (LPARAM) szChosenFont);

					HWND hslider = GetDlgItem(hDlg, IDC_TEXTURE);
					xdiv = ydiv = (int) SendMessage(hslider, TBM_GETPOS, 0, 0);

					bComplete = IsDlgButtonChecked(hDlg, IDC_COMPLETE);
					if (!bComplete)
					{
						bBold = IsDlgButtonChecked(hDlg, IDC_BOLD);
						bItalic = IsDlgButtonChecked(hDlg, IDC_ITALIC);
						//bUnderline = IsDlgButtonChecked(hDlg, IDC_UNDERLINE);
					}
					else
					{
						bBold = bItalic = bUnderline = FALSE;
					}
				}
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
			else if (LOWORD(wParam) == IDC_SCRIPT)
			{
				OnFontScript(hDlg);
			}
			else if (LOWORD(wParam) == IDC_COMPLETE)
			{
				BOOL isComplete = IsDlgButtonChecked(hDlg, IDC_COMPLETE);
				EnableWindow(GetDlgItem(hDlg, IDC_BOLD), isComplete ? FALSE : TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_ITALIC), isComplete ? FALSE : TRUE);
				EnableWindow(GetDlgItem(hDlg, IDC_UNDERLINE), isComplete ? FALSE : TRUE);
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

///////////////////////////////////////////////////////////
// Main dialog

static void UpdateDisplays(HWND hDlg)
{
	TCHAR szMod[4] = {};
	swprintf_s(szMod, _T("%d"), xdiv);
	if (bBold)
		wcscat_s(szMod, _T("b"));
	if (bItalic)
		wcscat_s(szMod, _T("i"));

	TCHAR szDisplay[80] = {};
	if (szChosenFont[0] != 0)
		swprintf_s(szDisplay, _T("%s [%s] [%s]"), szChosenFont, GetTitle(szFontScript), szMod);
	HWND hDisplay = GetDlgItem(hDlg, IDC_FONT_DISPLAY);
	SetWindowText(hDisplay, szDisplay);

	hDisplay = GetDlgItem(hDlg, IDC_ASSET_DISPLAY);
	SetWindowText(hDisplay, GetTitle(szAssetScript));
}

static void OnInit(HWND hdlg)
{
	CenterWindow(hdlg);

	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PACKAGEMAKER));
	SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

	GetRegistryStr(_T("font"), szChosenFont, _countof(szChosenFont));
	GetRegistryStr(_T("fontscript"), szFontScript, _countof(szFontScript));
	GetRegistryStr(_T("assetscript"), szAssetScript, _countof(szAssetScript));
	xdiv = GetRegistryInt(_T("xdiv"), xdiv);
	ydiv = GetRegistryInt(_T("ydiv"), ydiv);
	bBold = (GetRegistryInt(_T("bold"), bBold) ? TRUE : FALSE);
	bItalic = (GetRegistryInt(_T("italic"), bItalic) ? TRUE : FALSE);

	UpdateDisplays(hdlg);
}

static void OnFont(HWND hDlg)
{
	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_FONT), hDlg, FontProc) == IDOK)
	{
		SetRegistryStr(_T("font"), szChosenFont, _tcsclen(szChosenFont) + 1);
		SetRegistryStr(_T("fontscript"), szFontScript, _tcsclen(szFontScript) + 1);
		SetRegistryInt(_T("xdiv"), xdiv);
		SetRegistryInt(_T("ydiv"), ydiv);
		SetRegistryInt(_T("bold"), bBold);
		SetRegistryInt(_T("italic"), bItalic);

		UpdateDisplays(hDlg);
	}
}

static void OnAsset(HWND hDlg)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Script Files (*.txt)\0*.txt\0\0");
	ofn.lpstrTitle = _T("Script Chooser");
	ofn.lpstrFile = szAssetScript;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		SetRegistryStr(_T("assetscript"), szAssetScript, _tcslen(szAssetScript) + 1);
		UpdateDisplays(hDlg);
	}
}

static void OnCreateStart(HWND hDlg)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_FONT);
	EnableWindow(hBtn, FALSE);
	hBtn = GetDlgItem(hDlg, IDC_OTHER);
	EnableWindow(hBtn, FALSE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Abort"));

	HWND hstatus = GetDlgItem(hDlg, IDC_STATUS);
	SetWindowText(hstatus, L"");
}

static void OnCreateFinish(HWND hDlg, DWORD wParam)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_FONT);
	EnableWindow(hBtn, TRUE);
	hBtn = GetDlgItem(hDlg, IDC_OTHER);
	EnableWindow(hBtn, TRUE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Quit"));

	if (wParam)
	{
		HWND hstatus = GetDlgItem(hDlg, IDC_STATUS);
		SetWindowText(hstatus, L"Complete");
	}

	threadID = 0;
}

static void OnCreateUpdate(HWND hDlg, WPARAM wParam)
{
	TCHAR* pupdate = (TCHAR*)wParam;

	HWND hStatus = GetDlgItem(hDlg, IDC_STATUS);
	SetWindowText(hStatus, pupdate);
	delete[] pupdate;
}

static void OnFontGo(HWND hDlg)
{
	CREATE_FONT_DATA* pdata = new CREATE_FONT_DATA;
	pdata->hdlg = hDlg;
	_tcscpy_s(pdata->szFont, 80, szChosenFont);
	_tcscpy_s(pdata->szScript, MAX_PATH, szFontScript);
	_tcscpy_s(pdata->szOutput, MAX_PATH, _szDefaultFontPkg);
	pdata->xtxtdiv = xdiv;
	pdata->ytxtdiv = ydiv;
	pdata->nWeight = (bBold ? FW_BOLD : FW_NORMAL);
	pdata->bItalic = bItalic;
	pdata->bUnderline = bUnderline;
	pdata->bComplete = bComplete;

	CreateThread(NULL, 0, CreateFontProc, (LPVOID)pdata, 0, &threadID);
}

static void OnAssetGo(HWND hDlg)
{
	CREATE_ASSET_DATA* pdata = new CREATE_ASSET_DATA;
	pdata->hdlg = hDlg;
	_tcscpy_s(pdata->szScript, MAX_PATH, szAssetScript);

	CreateAssetProc((LPVOID)pdata);
	OnCreateFinish(hDlg, 1);
}

static BOOL SplitAssignment(const string& line, string& key, string& value)
{
	size_t equal = line.find('=');
	if (equal == string::npos)
		return FALSE;
	key = line.substr(0, equal);
	value = line.substr(equal + 1);
	return TRUE;
}

static bool IsEmptyLine(const string& line)
{
	return (line.empty());
}

static bool IsCommentedLine(const string& line)
{
	return (!line.empty() && line[0] == '#');
}

static BOOL ProcessFontSetting(fstream& infile, FONT_SETTING& fontSetting, const char* pathPrefix)
{
	string buffer, key, value;
	while (getline(infile, buffer))
	{
		if (IsEmptyLine(buffer))
			break;
		if (IsCommentedLine(buffer))
			continue;
		if (SplitAssignment(buffer, key, value))
		{
			if (key == "filename")
				fontSetting.filename = pathPrefix + value;
			else if (key == "xdiv")
				fontSetting.xdiv = atoi(value.c_str());
			else if (key == "ydiv")
				fontSetting.ydiv = atoi(value.c_str());
			else if (key == "bold")
				fontSetting.bold = (value == "true" ? true : false);
			else if (key == "italic")
				fontSetting.italic = (value == "true" ? true : false);
			else if (key == "underline")
				fontSetting.underline = (value == "true" ? true : false);
			else if (key == "complete")
				fontSetting.complete = (value == "true" ? true : false);
			else
				return FALSE;
		}
		else
			return FALSE;
	}
	return TRUE;
}

static CREATE_BATCH_DATA* ProcessBatchConfig(const TCHAR* path)
{
	char szPathPrefix[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, GetPath(path), -1, szPathPrefix, _countof(szPathPrefix), NULL, NULL);

	fstream infile;
	infile.open(path, ios_base::in);
	if (!infile.is_open())
		return NULL;

	CREATE_BATCH_DATA* pdata = new CREATE_BATCH_DATA;
	FONT_SETTING fontDefaultSetting;
	fontDefaultSetting.xdiv = fontDefaultSetting.ydiv = 1;
	fontDefaultSetting.bold = fontDefaultSetting.italic = fontDefaultSetting.underline = false;

	string buffer;
	while (getline(infile, buffer))
	{
		if (IsEmptyLine(buffer) || IsCommentedLine(buffer))
			continue;

		if (buffer == ":font-default-settings")
		{
			if (!ProcessFontSetting(infile, fontDefaultSetting, szPathPrefix))
				break;
		}
		else if (buffer == ":font-scripts")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				string script(szPathPrefix);
				script.append(buffer);
				pdata->fontScripts.push_back(script);
			}
		}
		else if (buffer == ":asset-scripts")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				string script(szPathPrefix);
				script.append(buffer);
				pdata->assetScripts.push_back(script);
			}
		}
		else if (buffer[0] == ':')
		{
			FONT_SETTING newFontSetting = fontDefaultSetting;
			newFontSetting.font = buffer.substr(1);
			if (ProcessFontSetting(infile, newFontSetting, szPathPrefix))
				pdata->fontSettings.push_back(newFontSetting);
			else
				break;
		}
	}

	infile.close();

	return pdata;
}

static void OnBatch(HWND hDlg)
{
	TCHAR szBatchPath[MAX_PATH] = {};

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Batch Configuration Files (*.cfg)\0*.cfg\0\0");
	ofn.lpstrTitle = _T("Batch Chooser");
	ofn.lpstrFile = szBatchPath;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		CREATE_BATCH_DATA* pdata = ProcessBatchConfig(szBatchPath);
		if (pdata != NULL)
		{
			pdata->hdlg = hDlg;
			CreateThread(NULL, 0, CreateBatchProc, (LPVOID)pdata, 0, &threadID);
		}
	}
}

static void OnQuit(HWND hDlg)
{
	PostQuitMessage(0);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnInit(hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_FONT)
			{
				OnFont(hDlg);
			}
			else if (LOWORD(wParam) == IDC_ASSET)
			{
				OnAsset(hDlg);
			}
			else if (LOWORD(wParam) == IDC_FONT_GO)
			{
				OnFontGo(hDlg);
			}
			else if (LOWORD(wParam) == IDC_ASSET_GO)
			{
				OnAssetGo(hDlg);
			}
			else if (LOWORD(wParam) == IDC_BATCH)
			{
				OnBatch(hDlg);
			}
			else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				OnQuit(hDlg);
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
		break;

	case WM_CREATE_START:
		OnCreateStart(hDlg);
		break;

	case WM_CREATE_FINISH:
		OnCreateFinish(hDlg, wParam);
		break;

	case WM_CREATE_UPDATE:
		OnCreateUpdate(hDlg, wParam);
		break;
	}
	return (INT_PTR)FALSE;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   //hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
   //   CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
   hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PACKAGEMAKER_DIALOG), NULL, DlgProc);
   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PACKAGEMAKER, szWindowClass, MAX_LOADSTRING);
//	MyRegisterClass(hInstance);

	// Perform application initialization:
	InitRegistry();
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PACKAGEMAKER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}
