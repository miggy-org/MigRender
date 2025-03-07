// PackageViewer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <fstream>
using namespace std;

#include "PackageViewer.h"
#include "Reg.h"

#include "rendbitmap.h"
#include "rendthread.h"
#include "model.h"
#include "jpegio.h"

#define MAX_LOADSTRING	100

#define TIMER_ID		1000
#define TIMER_DELAY		500

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
DWORD threadID = 0;
RECT minClientRect = { 0, 0, 0, 0 };
RECT minBitmapRect = { 0, 0, 0, 0 };
CRendBitmap bmp;

TCHAR szPackage[MAX_PATH] = {};					// path to package to test
TCHAR szScript[MAX_PATH] = {};					// path to font script to test
TCHAR szBackdrop[MAX_PATH] = {};				// path to backdrop model to test
TCHAR szLights[MAX_PATH] = {};					// path to lights model to test
TCHAR szTexture1[MAX_PATH] = {};				// path to texture map 1
TCHAR szTexture2[MAX_PATH] = {};				// path to texture map 2
TCHAR szReflect[MAX_PATH] = {};					// path to reflection map
TCHAR szBackdropTexture1[MAX_PATH] = {};		// path to texture map 1
TCHAR szBackdropTexture2[MAX_PATH] = {};		// path to texture map 2
TCHAR szBackdropReflect[MAX_PATH] = {};			// path to reflection map
TCHAR szUserImage[MAX_PATH] = {};				// path to user selected image
BOOL bTexture1 = TRUE;							// texture map 1 enable
BOOL bTexture2 = FALSE;							// texture map 2 enable
BOOL bReflect = FALSE;							// reflection map enable
BOOL bBackdropTexture1 = TRUE;					// backdrop texture map 1 enable
BOOL bBackdropTexture2 = FALSE;					// backdrop texture map 2 enable
BOOL bBackdropReflect = FALSE;					// backdrop reflection map enable
BOOL bUserImage = FALSE;						// user image enable

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

	SetWindowPos(hWnd, NULL, x, y, 800, 600, SWP_NOZORDER);
}

static void UpdateField(HWND hdlg, int idEdit, TCHAR* szPath)
{
	if (szPath[0] != 0)
	{
		TCHAR* p = wcsrchr(szPath, _T('\\'));
		if (p)
			p++;
		else
			p = szPath;
		SetDlgItemText(hdlg, idEdit, p);
	}
}

static void UpdateEdits(HWND hdlg)
{
	UpdateField(hdlg, IDC_TEXTURE1, szTexture1);
	UpdateField(hdlg, IDC_TEXTURE2, szTexture2);
	UpdateField(hdlg, IDC_REFLECT, szReflect);
	UpdateField(hdlg, IDC_BDTEXTURE1, szBackdropTexture1);
	UpdateField(hdlg, IDC_BDTEXTURE2, szBackdropTexture2);
	UpdateField(hdlg, IDC_BDREFLECT, szBackdropReflect);
	UpdateField(hdlg, IDC_USERIMAGE, szUserImage);
}

static void OnInitTexturesDialog(HWND hdlg)
{
	//CenterWindow(hdlg);

	CheckDlgButton(hdlg, IDC_ENABLE_TEXTURE1, bTexture1);
	CheckDlgButton(hdlg, IDC_ENABLE_TEXTURE2, bTexture2);
	CheckDlgButton(hdlg, IDC_ENABLE_REFLECT, bReflect);
	CheckDlgButton(hdlg, IDC_ENABLE_BDTEXTURE1, bBackdropTexture1);
	CheckDlgButton(hdlg, IDC_ENABLE_BDTEXTURE2, bBackdropTexture2);
	CheckDlgButton(hdlg, IDC_ENABLE_BDREFLECT, bBackdropReflect);

	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_TEXTURE1), bTexture1);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_TEXTURE2), bTexture2);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_REFLECT), bReflect);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_BDTEXTURE1), bBackdropTexture1);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_BDTEXTURE2), bBackdropTexture2);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_BDREFLECT), bBackdropReflect);
	EnableWindow(GetDlgItem(hdlg, IDC_CHANGE_USERIMAGE), bUserImage);

	UpdateEdits(hdlg);
}

static BOOL OnEnableChecked(HWND hdlg, int idEnableCtrl, int idChangeBtn)
{
	BOOL bEnabled = IsDlgButtonChecked(hdlg, idEnableCtrl);
	HWND hitem = GetDlgItem(hdlg, idChangeBtn);
	EnableWindow(hitem, bEnabled);
	return bEnabled;
}

static void OnChangeImage(HWND hdlg, TCHAR* szPath, int nChars)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hdlg;
	ofn.lpstrFilter = _T("JPEG Files (*.jpg, *.jpeg)\0*.jpg;*.jpeg\0PNG Files(*.png)\0*.png\0\0");
	ofn.lpstrTitle = _T("Image chooser");
	ofn.lpstrFile = szPath;
	ofn.nMaxFile = nChars;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		UpdateEdits(hdlg);
	}
}

INT_PTR CALLBACK TexturesProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnInitTexturesDialog(hDlg);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_ENABLE_TEXTURE1)
			{
				bTexture1 = OnEnableChecked(hDlg, IDC_ENABLE_TEXTURE1, IDC_CHANGE_TEXTURE1);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_TEXTURE2)
			{
				bTexture2 = OnEnableChecked(hDlg, IDC_ENABLE_TEXTURE2, IDC_CHANGE_TEXTURE2);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_REFLECT)
			{
				bReflect = OnEnableChecked(hDlg, IDC_ENABLE_REFLECT, IDC_CHANGE_REFLECT);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_BDTEXTURE1)
			{
				bBackdropTexture1 = OnEnableChecked(hDlg, IDC_ENABLE_BDTEXTURE1, IDC_CHANGE_BDTEXTURE1);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_BDTEXTURE2)
			{
				bBackdropTexture2 = OnEnableChecked(hDlg, IDC_ENABLE_BDTEXTURE2, IDC_CHANGE_BDTEXTURE2);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_BDREFLECT)
			{
				bBackdropReflect = OnEnableChecked(hDlg, IDC_ENABLE_BDREFLECT, IDC_CHANGE_BDREFLECT);
			}
			else if (LOWORD(wParam) == IDC_ENABLE_USERIMAGE)
			{
				bUserImage = OnEnableChecked(hDlg, IDC_ENABLE_USERIMAGE, IDC_CHANGE_USERIMAGE);
			}
			else if (LOWORD(wParam) == IDC_CHANGE_TEXTURE1)
			{
				OnChangeImage(hDlg, szTexture1, _countof(szTexture1));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_TEXTURE2)
			{
				OnChangeImage(hDlg, szTexture2, _countof(szTexture2));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_REFLECT)
			{
				OnChangeImage(hDlg, szReflect, _countof(szReflect));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_BDTEXTURE1)
			{
				OnChangeImage(hDlg, szBackdropTexture1, _countof(szBackdropTexture1));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_BDTEXTURE2)
			{
				OnChangeImage(hDlg, szBackdropTexture2, _countof(szBackdropTexture2));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_BDREFLECT)
			{
				OnChangeImage(hDlg, szBackdropReflect, _countof(szBackdropReflect));
			}
			else if (LOWORD(wParam) == IDC_CHANGE_USERIMAGE)
			{
				OnChangeImage(hDlg, szUserImage, _countof(szUserImage));
			}
			else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				EndDialog(hDlg, LOWORD(wParam));
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

static void OnInit(HWND hdlg)
{
	CenterWindow(hdlg);

	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_PACKAGEVIEWER));
	SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);

	if (!GetRegistryStr(_T("package"), szPackage, _countof(szPackage)))
		wcscpy_s(szPackage, _countof(szPackage), _T("..\\models\\arial.pkg"));
	if (!GetRegistryStr(_T("script"), szScript, _countof(szScript)))
		wcscpy_s(szScript, _countof(szScript), _T(""));
	if (!GetRegistryStr(_T("backdrop"), szBackdrop, _countof(szBackdrop)))
		wcscpy_s(szBackdrop, _countof(szBackdrop), _T("..\\models\\backdrop-slab.mdl"));
	if (!GetRegistryStr(_T("lights"), szLights, _countof(szLights)))
		wcscpy_s(szLights, _countof(szLights), _T("..\\models\\lights-many.mdl"));

	bTexture1 = (GetRegistryInt(_T("texture1-enabled"), bTexture1) > 0 ? TRUE : FALSE);
	bTexture2 = (GetRegistryInt(_T("texture2-enabled"), bTexture2) > 0 ? TRUE : FALSE);
	bReflect = (GetRegistryInt(_T("reflect-enabled"), bReflect) > 0 ? TRUE : FALSE);
	bBackdropTexture1 = (GetRegistryInt(_T("bdtexture1-enabled"), bBackdropTexture1) > 0 ? TRUE : FALSE);
	bBackdropTexture2 = (GetRegistryInt(_T("bdtexture2-enabled"), bBackdropTexture2) > 0 ? TRUE : FALSE);
	bBackdropReflect = (GetRegistryInt(_T("bdreflect-enabled"), bBackdropReflect) > 0 ? TRUE : FALSE);
	bUserImage = (GetRegistryInt(_T("userimage-enabled"), bUserImage) > 0 ? TRUE : FALSE);

	if (!GetRegistryStr(_T("texture1"), szTexture1, _countof(szTexture1)))
		wcscpy_s(szTexture1, _countof(szTexture1), _T("..\\images\\wood.jpg"));
	if (!GetRegistryStr(_T("texture2"), szTexture2, _countof(szTexture2)))
		wcscpy_s(szTexture2, _countof(szTexture2), _T(""));
	if (!GetRegistryStr(_T("reflect"), szReflect, _countof(szReflect)))
		wcscpy_s(szReflect, _countof(szReflect), _T("..\\images\\reflect1.jpg"));
	if (!GetRegistryStr(_T("bdtexture1"), szBackdropTexture1, _countof(szBackdropTexture1)))
		wcscpy_s(szBackdropTexture1, _countof(szBackdropTexture1), _T("..\\images\\marble2.jpg"));
	if (!GetRegistryStr(_T("bdtexture2"), szBackdropTexture2, _countof(szBackdropTexture2)))
		wcscpy_s(szBackdropTexture2, _countof(szBackdropTexture2), _T(""));
	if (!GetRegistryStr(_T("bdreflect"), szBackdropReflect, _countof(szBackdropReflect)))
		wcscpy_s(szBackdropReflect, _countof(szBackdropReflect), _T("..\\images\\reflect2.jpg"));
	if (!GetRegistryStr(_T("userimage"), szUserImage, _countof(szUserImage)))
		wcscpy_s(szUserImage, _countof(szUserImage), _T(""));

	TCHAR szText[80] = {};
	if (!GetRegistryStr(_T("text"), szText, _countof(szText)))
		wcscpy_s(szText, _countof(szText), _T("hello"));
	SetDlgItemText(hdlg, IDC_TEXT, szText);

	TCHAR szSuffix[20] = {};
	if (!GetRegistryStr(_T("suffix"), szSuffix, _countof(szSuffix)))
		wcscpy_s(szSuffix, _countof(szSuffix), _T(""));
	SetDlgItemText(hdlg, IDC_SUFFIX, szSuffix);
}

static POINT refOKCorner = { 0, 0 };
static POINT refRenderCorner = { 0, 0 };
static POINT refBatchCorner = { 0, 0 };

static void MoveControl(HWND hdlg, int idCtrl, int newBmpWidth, int newBmpHeight, POINT& refCorner, BOOL moveX)
{
	RECT rect;
	HWND hbtn = GetDlgItem(hdlg, idCtrl);
	GetWindowRect(hbtn, &rect);
	POINT corner;
	corner.x = rect.left;
	corner.y = rect.top;
	ScreenToClient(hdlg, &corner);
	if (refCorner.x == 0)
		refCorner = corner;

	if (moveX)
		corner.x = refCorner.x + newBmpWidth - minBitmapRect.right;
	corner.y = refCorner.y + newBmpHeight - minBitmapRect.bottom;
	SetWindowPos(hbtn, NULL, corner.x, corner.y, 0, 0, SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

static void OnSize(HWND hdlg)
{
	if (threadID == 0)
	{
		RECT clientRect;
		GetClientRect(hdlg, &clientRect);
		if (minClientRect.right == 0)
			minClientRect = clientRect;

		RECT bitmapRect;
		HWND hitem = GetDlgItem(hdlg, IDC_OUTPUT);
		GetClientRect(hitem, &bitmapRect);
		if (minBitmapRect.right == 0)
			minBitmapRect = bitmapRect;

		int newWidth = (clientRect.right - (minClientRect.right - minBitmapRect.right));
		int newHeight = (clientRect.bottom - (minClientRect.bottom - minBitmapRect.bottom));
		int bmpWidth = max(newWidth, minBitmapRect.right);
		int bmpHeight = max(newHeight, minBitmapRect.bottom);
		SetWindowPos(hitem, NULL, 0, 0, bmpWidth, bmpHeight, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);

		MoveControl(hdlg, IDOK, bmpWidth, bmpHeight, refOKCorner, TRUE);
		MoveControl(hdlg, IDC_PACKAGE, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_SCRIPT, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_TEXT, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_SUFFIX, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_BACKDROP, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_LIGHTS, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_TEXTURES, bmpWidth, bmpHeight, refOKCorner, FALSE);
		MoveControl(hdlg, IDC_RENDER, bmpWidth, bmpHeight, refRenderCorner, TRUE);
		MoveControl(hdlg, IDC_BATCH, bmpWidth, bmpHeight, refBatchCorner, TRUE);

		HDC hDC = GetDC(hdlg);
		bmp.Init(bmpWidth, bmpHeight, hDC);
		ReleaseDC(hdlg, hDC);
		SendMessage(hitem, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)bmp.GetBitmap());
	}
}

static void OnPackage(HWND hDlg)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Package Files (*.pkg)\0*.pkg\0\0");
	ofn.lpstrTitle = _T("Pick a package, baby!");
	ofn.lpstrFile = szPackage;
	ofn.nMaxFile = _countof(szPackage);
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		SetRegistryStr(_T("package"), szPackage, (DWORD) wcslen(szPackage) + 1);
	}
}

static void OnScript(HWND hDlg)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Script Files (*.txt)\0*.txt\0\0");
	ofn.lpstrTitle = _T("Pick a script, baby!");
	ofn.lpstrFile = szScript;
	ofn.nMaxFile = _countof(szScript);
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		SetRegistryStr(_T("script"), szScript, (DWORD)wcslen(szScript) + 1);
	}
	else
	{
		// cancel means no script
		szScript[0] = 0;
	}
}

static void OnModel(HWND hDlg, TCHAR* szModel, int nChars, const TCHAR* szRegEntry)
{
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Model Files (*.mdl)\0*.mdl\0\0");
	ofn.lpstrTitle = _T("Pick a model, baby!");
	ofn.lpstrFile = szModel;
	ofn.nMaxFile = nChars;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (!GetOpenFileName(&ofn))
	{
		szModel[0] = 0;
	}
	SetRegistryStr(szRegEntry, szModel, (DWORD)wcslen(szModel) + 1);
}

static void OnTextures(HWND hDlg)
{
	DialogBox(hInst, MAKEINTRESOURCE(IDD_TEXTURES_DIALOG), hDlg, TexturesProc);
}

static void CopyImageParam(BOOL doCopy, const TCHAR* szImage, char* szDest, int nChars, const TCHAR* szRegEnable, const TCHAR* szRegImage)
{
	if (doCopy)
		WideCharToMultiByte(CP_UTF8, 0, szImage, -1, szDest, nChars, NULL, NULL);
	else
		szDest[0] = 0;
	SetRegistryInt(szRegEnable, doCopy);
	SetRegistryStr(szRegImage, szImage, _tcsclen(szImage) + 1);
}

static void OnRender(HWND hDlg)
{
	REND_THREAD_DATA* pdata = new REND_THREAD_DATA;
	TCHAR buf[MAX_PATH];

	pdata->hdlg = hDlg;
	pdata->pbmp = &bmp;

	HWND hitem = GetDlgItem(hDlg, IDC_TEXT);
	GetWindowText(hitem, buf, MAX_PATH);
	SetRegistryStr(_T("text"), buf, (DWORD)wcslen(buf) + 1);
	WideCharToMultiByte(CP_UTF8, 0, buf, -1, pdata->szText, sizeof(pdata->szText), NULL, NULL);

	TCHAR suffix[20];
	GetWindowText(GetDlgItem(hDlg, IDC_SUFFIX), suffix, 20);
	SetRegistryStr(_T("suffix"), suffix, (DWORD)wcslen(suffix) + 1);
	WideCharToMultiByte(CP_UTF8, 0, suffix, -1, pdata->szLookupSuffix, sizeof(pdata->szLookupSuffix), NULL, NULL);

	WideCharToMultiByte(CP_UTF8, 0, szPackage, -1, pdata->szPackage, sizeof(pdata->szPackage), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szScript, -1, pdata->szScript, sizeof(pdata->szScript), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szBackdrop, -1, pdata->szBackdrop, sizeof(pdata->szBackdrop), NULL, NULL);
	WideCharToMultiByte(CP_UTF8, 0, szLights, -1, pdata->szLights, sizeof(pdata->szLights), NULL, NULL);

	CopyImageParam(bTexture1, szTexture1, pdata->szTexture1, _countof(pdata->szTexture1), _T("texture1-enabled"), _T("texture1"));
	CopyImageParam(bTexture2, szTexture2, pdata->szTexture2, _countof(pdata->szTexture2), _T("texture2-enabled"), _T("texture2"));
	CopyImageParam(bReflect, szReflect, pdata->szReflect, _countof(pdata->szReflect), _T("reflect-enabled"), _T("reflect"));
	CopyImageParam(bBackdropTexture1, szBackdropTexture1, pdata->szBackdropTexture1, _countof(pdata->szBackdropTexture1), _T("bdtexture1-enabled"), _T("bdtexture1"));
	CopyImageParam(bBackdropTexture2, szBackdropTexture2, pdata->szBackdropTexture2, _countof(pdata->szBackdropTexture2), _T("bdtexture2-enabled"), _T("bdtexture2"));
	CopyImageParam(bBackdropReflect, szBackdropReflect, pdata->szBackdropReflect, _countof(pdata->szBackdropReflect), _T("bdreflect-enabled"), _T("bdreflect"));
	CopyImageParam(bUserImage, szUserImage, pdata->szUserImage, _countof(pdata->szUserImage), _T("userimage-enabled"), _T("userimage"));

	CreateThread(NULL, 0, RendThreadProc, (LPVOID) pdata, 0, &threadID);
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

static bool IsEmptyLine(const string& line)
{
	return (line.empty());
}

static bool IsCommentedLine(const string& line)
{
	return (!line.empty() && line[0] == '#');
}

static BOOL SplitAssignment(const string& line, string& key, string& value, char delim)
{
	size_t equal = line.find(delim);
	if (equal == string::npos)
		return FALSE;
	key = line.substr(0, equal);
	value = line.substr(equal + 1);
	return TRUE;
}

static size_t ParseList(const string& line, const char* delim, vector<string>& out)
{
	char szTmp[MAX_PATH];
	strcpy_s(szTmp, line.c_str());

	char* context = NULL;
	char* token = strtok_s(szTmp, delim, &context);
	while (token != NULL)
	{
		if (!_stricmp(token, "[comma]"))
			out.push_back(",");
		else if (!_stricmp(token, "[empty]"))
			out.push_back("");
		else
			out.push_back(token);
		token = strtok_s(NULL, delim, &context);
	}

	return out.size();
}

static BATCH_THREAD_DATA* ProcessBatchConfig(const TCHAR* path)
{
	char szPathPrefix[MAX_PATH];
	WideCharToMultiByte(CP_UTF8, 0, GetPath(path), -1, szPathPrefix, _countof(szPathPrefix), NULL, NULL);

	fstream infile;
	infile.open(path, ios_base::in);
	if (!infile.is_open())
		return NULL;

	BATCH_THREAD_DATA* pdata = new BATCH_THREAD_DATA;

	string buffer;
	while (getline(infile, buffer))
	{
		if (IsEmptyLine(buffer) || IsCommentedLine(buffer))
			continue;

		if (buffer == ":packages")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				pdata->packages.push_back(buffer);
			}
		}
		else if (buffer == ":prefixes")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				vector<string> list;
				if (ParseList(buffer, ", ", list) > 0)
					pdata->prefixes.insert(pdata->prefixes.end(), list.begin(), list.end());
			}
		}
		else if (buffer == ":suffixes")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				vector<string> list;
				if (ParseList(buffer, ", ", list) > 0)
					pdata->suffixes.insert(pdata->suffixes.end(), list.begin(), list.end());
			}
		}
		else if (buffer == ":scripts")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				pdata->fontScripts.push_back(buffer);
			}
		}
		else if (buffer == ":settings")
		{
			while (getline(infile, buffer))
			{
				if (IsEmptyLine(buffer))
					break;
				if (IsCommentedLine(buffer))
					continue;
				string key, value;
				if (SplitAssignment(buffer, key, value, '='))
				{
					if (key == "dimen")
					{
						string width, height;
						if (SplitAssignment(value, width, height, 'x'))
						{
							pdata->width = atoi(width.c_str());
							pdata->height= atoi(height.c_str());
						}
					}
					else if (key == "setup")
					{
						vector<string> list;
						if (ParseList(value, ", ", list) > 0)
							pdata->setupScripts.insert(pdata->setupScripts.end(), list.begin(), list.end());
					}
					else if (key == "dest")
					{
						pdata->dest = value;
					}
				}
			}
		}
	}

	infile.close();

	CopyImageParam(bTexture1, szTexture1, pdata->szTexture1, _countof(pdata->szTexture1), _T("texture1-enabled"), _T("texture1"));
	CopyImageParam(bTexture2, szTexture2, pdata->szTexture2, _countof(pdata->szTexture2), _T("texture2-enabled"), _T("texture2"));
	CopyImageParam(bReflect, szReflect, pdata->szReflect, _countof(pdata->szReflect), _T("reflect-enabled"), _T("reflect"));

	return pdata;
}

static void OnBatch(HWND hDlg)
{
	TCHAR szBatch[MAX_PATH];
	_tcscpy_s(szBatch, _T(".\\batch.cfg"));

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hDlg;
	ofn.lpstrFilter = _T("Batch Config Files (*.cfg)\0*.cfg\0\0");
	ofn.lpstrTitle = _T("Pick a batch, baby!");
	ofn.lpstrFile = szBatch;
	ofn.nMaxFile = _countof(szBatch);
	ofn.Flags = OFN_DONTADDTORECENT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		BATCH_THREAD_DATA* pdata = ProcessBatchConfig(szBatch);
		pdata->hdlg = hDlg;
		pdata->pbmp = &bmp;

		CreateThread(NULL, 0, BatchThreadProc, (LPVOID)pdata, 0, &threadID);
	}
}

static void OnRenderStart(HWND hDlg)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_RENDER);
	EnableWindow(hBtn, FALSE);
	hBtn = GetDlgItem(hDlg, IDC_BATCH);
	EnableWindow(hBtn, FALSE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Abort"));

	SetTimer(hDlg, TIMER_ID, TIMER_DELAY, NULL);
}

static void OnRenderFinish(HWND hDlg, DWORD dwElapsed)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_RENDER);
	EnableWindow(hBtn, TRUE);
	hBtn = GetDlgItem(hDlg, IDC_BATCH);
	EnableWindow(hBtn, TRUE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Quit"));

	KillTimer(hDlg, TIMER_ID);
	InvalidateRect(hDlg, NULL, FALSE);

	threadID = 0;
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
		OnSize(hDlg);
		OnInit(hDlg);
		return (INT_PTR)TRUE;

	case WM_SIZE:
		OnSize(hDlg);
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_PACKAGE)
			{
				OnPackage(hDlg);
			}
			else if (LOWORD(wParam) == IDC_SCRIPT)
			{
				OnScript(hDlg);
			}
			else if (LOWORD(wParam) == IDC_BACKDROP)
			{
				OnModel(hDlg, szBackdrop, sizeof(szBackdrop) / sizeof(TCHAR), _T("backdrop"));
			}
			else if (LOWORD(wParam) == IDC_LIGHTS)
			{
				OnModel(hDlg, szLights, sizeof(szLights) / sizeof(TCHAR), _T("lights"));
			}
			else if (LOWORD(wParam) == IDC_TEXTURES)
			{
				OnTextures(hDlg);
			}
			else if (LOWORD(wParam) == IDC_RENDER)
			{
				OnRender(hDlg);
				InvalidateRect(hDlg, NULL, FALSE);
			}
			else if (LOWORD(wParam) == IDC_BATCH)
			{
				OnBatch(hDlg);
			}
			else if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
			{
				if (threadID > 0)
				{
					bmp.SetStop();
				}
				else
				{
					OnQuit(hDlg);
					EndDialog(hDlg, LOWORD(wParam));
				}
				return (INT_PTR)TRUE;
			}
		}
		break;

	case WM_TIMER:
		if (wParam == TIMER_ID)
			InvalidateRect(hDlg, NULL, FALSE);
		break;

	case WM_REND_START:
		OnRenderStart(hDlg);
		break;

	case WM_REND_FINISH:
		OnRenderFinish(hDlg, (DWORD) wParam);
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
   hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_PACKAGEVIEWER_DIALOG), NULL, DlgProc);
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

	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_PACKAGEVIEWER, szWindowClass, MAX_LOADSTRING);
//	MyRegisterClass(hInstance);

	// Perform application initialization:
	InitRegistry();
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_PACKAGEVIEWER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CloseRegistry();
	return (int) msg.wParam;
}
