// TestApp.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "TestApp.h"
#include "Reg.h"

#include "rendbitmap.h"
#include "rendthread.h"
#include "model.h"
#include "jpegio.h"
#include "bmpio.h"

#define MAX_LOADSTRING	100

#define TIMER_ID		1000
#define TIMER_DELAY		500

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
CRendBitmap bmp;
DWORD threadID = 0;

// Globals for the resize stuff
int border = 0;
int delta_controls = 0;
int delta_text = 0;
//int delta_script = 0;
int delta_preview = 0;
int delta_render = 0;
int delta_quit = 0;

// only used during movie renders
bool ShouldAbortRender(void)
{
	return bmp.GetStop();
}

static void GetDeltas(HWND hDlg)
{
	RECT rdlg, rout, rtext, rpreview, rrender, rquit;
	GetWindowRect(hDlg, &rdlg);
	GetWindowRect(GetDlgItem(hDlg, IDC_OUTPUT), &rout);
	GetWindowRect(GetDlgItem(hDlg, IDC_TEXT), &rtext);
	//GetWindowRect(GetDlgItem(hDlg, IDC_SCRIPT), &rscript);
	GetWindowRect(GetDlgItem(hDlg, IDC_PREVIEW), &rpreview);
	GetWindowRect(GetDlgItem(hDlg, IDC_RENDER), &rrender);
	GetWindowRect(GetDlgItem(hDlg, IDOK), &rquit);

	border = rout.left - rdlg.left;
	delta_controls = rdlg.bottom - rtext.top;
	delta_text = rdlg.right - rtext.left;
	//delta_script = rdlg.right - rscript.left;
	delta_preview = rdlg.right - rpreview.left;
	delta_render = rdlg.right - rrender.left;
	delta_quit = rdlg.right - rquit.left;
}

static void CenterWindow(HWND hWnd)
{
	int width = GetRegistryInt(_T("width"));
	int height = GetRegistryInt(_T("height"));
	if (width == 0 || height == 0)
	{
		RECT rWnd;
		GetClientRect(hWnd, &rWnd);
		width = rWnd.right;
		height = rWnd.bottom;
	}

	int x = GetRegistryInt(_T("posx"));
	int y = GetRegistryInt(_T("posy"));
	if (x == 0 || y == 0)
	{
		int screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
		int screen_height = GetSystemMetrics(SM_CYFULLSCREEN);
		x = (screen_width - width) / 2;
		y = (screen_height - height) / 2;
	}

	SetWindowPos(hWnd, NULL, x, y, width, height, SWP_NOZORDER);
}

static void OnInit(HWND hdlg)
{
	GetDeltas(hdlg);
	CenterWindow(hdlg);

	HWND hEdit = GetDlgItem(hdlg, IDC_SCRIPT);
	if (hEdit)
	{
		TCHAR script[MAX_PATH];
		if (!GetRegistryStr(_T("script"), script, MAX_PATH))
			_tcscpy_s(script, MAX_PATH, _T("script.ini"));
		SetWindowText(hEdit, script);
	}

	HICON hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_TESTAPP));
	SendMessage(hdlg, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
}

static void OnSize(HWND hdlg)
{
	RECT rect;
	GetClientRect(hdlg, &rect);
	int width = rect.right;
	int height = rect.bottom;

	//HWND hitem = GetDlgItem(hdlg, IDC_LABEL);
	//SetWindowPos(hitem, NULL, width - delta_text, height - delta_controls + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	//hitem = GetDlgItem(hdlg, IDC_SCRIPT);
	//SetWindowPos(hitem, NULL, width - delta_script, height - delta_controls + 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	HWND hitem = GetDlgItem(hdlg, IDC_PREVIEW);
	SetWindowPos(hitem, NULL, width - delta_preview, height - delta_controls, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	hitem = GetDlgItem(hdlg, IDC_RENDER);
	SetWindowPos(hitem, NULL, width - delta_render, height - delta_controls, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	hitem = GetDlgItem(hdlg, IDOK);
	SetWindowPos(hitem, NULL, width - delta_quit, height - delta_controls, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	hitem = GetDlgItem(hdlg, IDC_TEXT);
	GetClientRect(hitem, &rect);
	SetWindowPos(hitem, NULL, border, height - delta_controls + 1, width - delta_preview - border - 10, rect.bottom, SWP_NOZORDER);

	hitem = GetDlgItem(hdlg, IDC_OUTPUT);
	SetWindowPos(hitem, NULL, border, border, width - 2*border, height - 2*border - delta_controls, SWP_NOZORDER);
	GetClientRect(hitem, &rect);
	HDC hDC = GetDC(hdlg);
	bmp.Init(rect.right - rect.left, rect.bottom - rect.top, hDC);
	ReleaseDC(hdlg, hDC);
	SendMessage(hitem, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) bmp.GetBitmap());
}

static void OnRender(HWND hDlg, BOOL doPreview)
{
	bmp.SetStop(false);

	HWND hEdit = GetDlgItem(hDlg, IDC_SCRIPT);
	if (hEdit)
	{
		TCHAR script[MAX_PATH];
		if (GetWindowText(hEdit, script, MAX_PATH) == 0)
			_tcscpy_s(script, MAX_PATH, _T("script.ini"));
		SetScriptFile(script);
	}

	//CBMPTarget* pbmpFile = new CBMPTarget();
	//pbmpFile->Init(620, 300, 32, "..\\images\\test.bmp");

	REND_TREAD_DATA* pdata = new REND_TREAD_DATA;
	pdata->hdlg = hDlg;
	pdata->hrend = GetDlgItem(hDlg, IDC_OUTPUT);
	pdata->preview = doPreview;
	pdata->ptarget = &bmp;
	//pdata->ptarget = pbmpFile;
	CreateThread(NULL, 0, RendThreadProc, (LPVOID)pdata, 0, &threadID);
}

static void OnRenderStart(HWND hDlg, BOOL useTimer)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_RENDER);
	EnableWindow(hBtn, FALSE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Abort"));

	if (useTimer)
		SetTimer(hDlg, TIMER_ID, TIMER_DELAY, NULL);
}

static void OnRenderFinish(HWND hDlg, DWORD dwElapsed, BOOL suppressInvalidate)
{
	HWND hBtn = GetDlgItem(hDlg, IDC_RENDER);
	EnableWindow(hBtn, TRUE);

	hBtn = GetDlgItem(hDlg, IDOK);
	SetWindowText(hBtn, _T("Quit"));

	TCHAR szTmp[40];
	hBtn = GetDlgItem(hDlg, IDC_TEXT);
	swprintf_s(szTmp, _T("Elapsed: %.3f seconds"), dwElapsed/1000.0);
	SetWindowText(hBtn, szTmp);

	KillTimer(hDlg, TIMER_ID);
	if (!suppressInvalidate)
		InvalidateRect(hDlg, NULL, FALSE);

	threadID = 0;
}

static void OnQuit(HWND hDlg)
{
	HWND hEdit = GetDlgItem(hDlg, IDC_SCRIPT);
	if (hEdit)
	{
		TCHAR script[MAX_PATH];
		if (GetWindowText(hEdit, script, MAX_PATH) > 0)
		{
			SetRegistryStr(_T("script"), script, (DWORD) (_tcslen(script) + 1));
		}
	}

	RECT rect;
	GetWindowRect(hDlg, &rect);
	SetRegistryInt(_T("width"), rect.right - rect.left);
	SetRegistryInt(_T("height"), rect.bottom - rect.top);
	SetRegistryInt(_T("posx"), rect.left);
	SetRegistryInt(_T("posy"), rect.top);

	PostQuitMessage(0);
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		OnInit(hDlg);
		OnSize(hDlg);
		return (INT_PTR)TRUE;

	case WM_SIZE:
		OnSize(hDlg);
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			if (LOWORD(wParam) == IDC_RENDER)
			{
				OnRender(hDlg, FALSE);
				InvalidateRect(hDlg, NULL, FALSE);
			}
			else if (LOWORD(wParam) == IDC_PREVIEW)
			{
				OnRender(hDlg, TRUE);
				InvalidateRect(hDlg, NULL, FALSE);
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

	case WM_MOUSEMOVE:
		if (threadID == 0)
		{
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			ClientToScreen(hDlg, &pt);

			HWND hitem = GetDlgItem(hDlg, IDC_OUTPUT);
			RECT rect;
			GetWindowRect(hitem, &rect);
			if (pt.x >= rect.left && pt.y >= rect.top && pt.x <= rect.right && pt.y <= rect.bottom)
			{
				ScreenToClient(hitem, &pt);
				pt.y = (rect.bottom - rect.top) - pt.y;

				int r, g, b;
				bmp.GetPixel(pt.x, pt.y, r, g, b);

				TCHAR szTmp[40];
				swprintf_s(szTmp, _T("X: %d Y: %d   R: %d G: %d B: %d"), pt.x, pt.y, r, g, b);
				hitem = GetDlgItem(hDlg, IDC_TEXT);
				SetWindowText(hitem, szTmp);
			}
		}
		break;

	case WM_LBUTTONDOWN:
		if (threadID == 0)
		{
			POINT pt;
			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			ClientToScreen(hDlg, &pt);

			HWND hitem = GetDlgItem(hDlg, IDC_OUTPUT);
			RECT rect;
			GetWindowRect(hitem, &rect);
			if (pt.x >= rect.left && pt.y >= rect.top && pt.x <= rect.right && pt.y <= rect.bottom)
			{
				ScreenToClient(hitem, &pt);
				pt.y = (rect.bottom - rect.top) - pt.y;

				REND_TREAD_DATA rendData;
				rendData.hdlg = hDlg;
				rendData.hrend = NULL;
				rendData.ptarget = &bmp;
				const TCHAR* szTmp = RendPixelDebug(&rendData, pt.x, pt.y);
				if (szTmp != NULL)
				{
					hitem = GetDlgItem(hDlg, IDC_TEXT);
					SetWindowText(hitem, szTmp);
				}
			}
		}
		break;
	
	case WM_TIMER:
		if (wParam == TIMER_ID)
			InvalidateRect(hDlg, NULL, FALSE);
		break;

	case WM_REND_START:
		OnRenderStart(hDlg, wParam ? FALSE : TRUE);
		break;

	case WM_REND_FINISH:
		OnRenderFinish(hDlg, (DWORD) wParam, (lParam ? TRUE : FALSE));
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
   hWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_TESTAPP_DIALOG), NULL, DlgProc);
   if (!hWnd)
      return FALSE;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

DWORD WINAPI TestConsole(LPVOID lpParam)
{
	AllocConsole();
	SetConsoleTitle(_T("MigRender Console"));

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_GRAYED);
	DrawMenuBar(GetConsoleWindow());

	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &consoleInfo);

	/*long stdioHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	int consoleHandleR = _open_osfhandle(stdioHandle, _O_TEXT);
	FILE* fptr = _fdopen(consoleHandleR, "r");
	*stdin = *fptr;
	setvbuf(stdin, NULL, _IONBF, 0);

	stdioHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	long consoleHandleW = _open_osfhandle(stdioHandle, _O_TEXT);
	fptr = _fdopen(consoleHandleW, "w");
	*stdout = *fptr;
	
	setvbuf(stdout, NULL, _IONBF, 0);

	stdioHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	*stderr = *fptr;
	setvbuf(stderr, NULL, _IONBF, 0);*/

	while (true)
	{
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), L"> ", 2, NULL, NULL);

		CONSOLE_READCONSOLE_CONTROL ictrl = {};
		ictrl.nLength = sizeof(CONSOLE_READCONSOLE_CONTROL);

		TCHAR buffer[80] = {};
		DWORD written = 0;
		ReadConsole(GetStdHandle(STD_INPUT_HANDLE), buffer, 80, &written, &ictrl);

		if (_tcsicmp(buffer, _T("quit\r\n")) == 0)
			break;
		if (_tcsicmp(buffer, _T("\r\n")) != 0)
			WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), buffer, written, NULL, NULL);
	}

	/*while (true)
	{
		std::cout << "> ";

		std::string line;
		std::getline(std::cin, line);

		if (line == "quit")
			break;

		std::cout << line;
		std::cout << "\n";
	}*/

	FreeConsole();
	return 0;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	//CreateThread(NULL, 0, TestConsole, NULL, 0, NULL);

	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_TESTAPP, szWindowClass, MAX_LOADSTRING);
//	MyRegisterClass(hInstance);

	// Perform application initialization:
	InitRegistry();
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTAPP));

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
