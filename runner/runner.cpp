// runner.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <string>
#include <sstream>
//#include <vector>
//#include <iterator>
#include <iostream>
//#include <cstdlib>
//#include <cstring>
#include <fstream>

#include <windows.h>

#include "migutil.h"
#include "model.h"
#include "bmpio.h"
#include "jpegio.h"
#include "pngio.h"
//#include "fileio.h"
//#include "package.h"

//#include "animmanage.h"
#include "parser2.h"
//#include "jsonio.h"
#include "targets.h"
#include "bitmaptarget.h"

#include "gl/oglrender.h"

using namespace std;
using namespace MigRender;

unique_ptr<MigRender::CModel> _model;
unique_ptr<MigRender::CAnimManager> _animManager;
unique_ptr<MigRender::CParser> _parser;
unique_ptr<CRendBitmap> _rendBitmap;

HWND _previewWnd = NULL;

HGLRC _hGLContext = NULL;
unique_ptr<OglRender> _oglRender;

// defined in shaders.cpp
extern string bgVertexShader;
extern string bgFragmentShader;
extern string mainVertexShader;
extern string mainFragmentShader;

// https://learn.microsoft.com/en-us/windows/console/reading-input-buffer-events
// https://stackoverflow.com/questions/4989127/getting-mouse-and-keyboard-inputs-in-console-c
// https://stackoverflow.com/questions/61919292/c-how-do-i-erase-a-line-from-the-console

static bool InitParser(void)
{
    _model = make_unique<CModel>();
    _animManager = make_unique<CAnimManager>();
    _parser = make_unique<CParser>();

    if (_oglRender != nullptr)
        _oglRender->ResetModel(true);
    if (_previewWnd != NULL)
        InvalidateRect(_previewWnd, NULL, TRUE);

    return _parser->Init(_model.get(), _animManager.get());
}

LRESULT CALLBACK PreviewWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);

        if (_rendBitmap != nullptr)
        {
            HDC hMemDC = CreateCompatibleDC(hDC);
            SelectObject(hMemDC, (*_rendBitmap).GetBitmap());
            BitBlt(hDC, 0, 0, (*_rendBitmap).GetWidth(), (*_rendBitmap).GetHeight(), hMemDC, 0, 0, SRCCOPY);
            DeleteDC(hMemDC);
        }
        else if (_hGLContext != NULL && _oglRender != nullptr)
        {
            wglMakeCurrent(hDC, _hGLContext);
            _oglRender->Render(*_model);
            SwapBuffers(hDC);
            wglMakeCurrent(hDC, NULL);
        }
        else
        {
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            BitBlt(hDC, 0, 0, clientRect.right, clientRect.bottom, NULL, 0, 0, BLACKNESS);
        }

        EndPaint(hWnd, &ps);
        return 0;
    }
    else if (uMsg == WM_SIZE)
    {
        if (_hGLContext != NULL)
        {
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);

            HDC hDC = GetDC(hWnd);
            wglMakeCurrent(hDC, _hGLContext);
            glViewport(0, 0, clientRect.right, clientRect.bottom);
            wglMakeCurrent(hDC, NULL);
        }
        if (_rendBitmap != nullptr)
        {
            _rendBitmap.release();
        }
        InvalidateRect(hWnd, NULL, TRUE);
    }
    else if (uMsg == WM_CLOSE)
    {
        if (_oglRender != nullptr)
        {
            _oglRender->Term();
            _oglRender.release();
        }
        wglDeleteContext(_hGLContext);
        _hGLContext = NULL;
        _previewWnd = NULL;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI RunPreviewWindow(LPVOID lpParam)
{
    HMODULE hMod = GetModuleHandle(NULL);

    WNDCLASS wc = {};
    wc.lpfnWndProc = PreviewWindowProc;
    wc.hInstance = hMod;
    wc.lpszClassName = L"MigRenderWindowClass";
    RegisterClass(&wc);

    LPRECT clientRect = static_cast<LPRECT>(lpParam);
    if (clientRect != NULL && clientRect->right > 0 && clientRect->bottom > 0)
        AdjustWindowRect(clientRect, WS_OVERLAPPEDWINDOW, FALSE);

    _previewWnd = CreateWindow(wc.lpszClassName, L"MigRender Preview", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        (clientRect != NULL && clientRect->right > 0 ? clientRect->right : CW_USEDEFAULT),
        (clientRect != NULL && clientRect->bottom > 0 ? clientRect->bottom : CW_USEDEFAULT),
        NULL, NULL, hMod, NULL);

    ShowWindow(_previewWnd, SW_NORMAL);
    UpdateWindow(_previewWnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

static bool InitPixelFormat(HDC hDC)
{
    PIXELFORMATDESCRIPTOR pfd =
    {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    // Flags
        PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
        32,                   // Colordepth of the framebuffer.
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        24,                   // Number of bits for the depthbuffer
        8,                    // Number of bits for the stencilbuffer
        0,                    // Number of Aux buffers in the framebuffer.
        PFD_MAIN_PLANE,
        0,
        0, 0, 0
    };
    int iPixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (iPixelFormat == 0)
    {
        DWORD err = GetLastError();
        return false;
    }
    if (!SetPixelFormat(hDC, iPixelFormat, &pfd))
    {
        DWORD err = GetLastError();
        return false;
    }
    return true;
}

static void OutputGLString(const char* prefix, GLenum name)
{
    OutputDebugStringA(prefix);
    OutputDebugStringA((const char*)glGetString(name));
    OutputDebugStringA("\n");
}

static string ReadShader(const string& path)
{
    ifstream infile;
    infile.open(path, ios_base::in);
    if (!infile.is_open())
        return "";

    // this is ok for small files
    return string(istreambuf_iterator<char>{infile}, {});
}

static bool InitGL(void)
{
    if (_previewWnd == NULL)
        return false;

    HDC hDC = ::GetDC(_previewWnd);
    if (!InitPixelFormat(hDC))
        return false;

    _hGLContext = wglCreateContext(hDC);
    if (_hGLContext == NULL)
    {
        DWORD err = GetLastError();
        return false;
    }
    wglMakeCurrent(hDC, _hGLContext);

    OutputGLString("Version: ", GL_VERSION);
    OutputGLString("Vendor: ", GL_VENDOR);
    OutputGLString("Renderer: ", GL_RENDERER);

    RECT clientRect;
    GetClientRect(_previewWnd, &clientRect);
    glViewport(0, 0, clientRect.right, clientRect.bottom);

    _oglRender = make_unique<OglRender>();
    _oglRender->Init();

    _oglRender->GetProgram(OglProgramType::Main)->BeginProgram()
        //.AddShader(OglShader(GL_VERTEX_SHADER, ReadShader("test.vs")))
        .AddShader(OglShader(GL_VERTEX_SHADER, mainVertexShader))
        //.AddShader(OglShader(GL_FRAGMENT_SHADER, ReadShader("test.fs")))
        .AddShader(OglShader(GL_FRAGMENT_SHADER, mainFragmentShader))
        .LinkProgram();

    _oglRender->GetProgram(OglProgramType::Bg)->BeginProgram()
        //.AddShader(OglShader(GL_VERTEX_SHADER, ReadShader("bg.vs")))
        .AddShader(OglShader(GL_VERTEX_SHADER, bgVertexShader))
        //.AddShader(OglShader(GL_FRAGMENT_SHADER, ReadShader("bg.fs")))
        .AddShader(OglShader(GL_FRAGMENT_SHADER, bgFragmentShader))
        .LinkProgram();

    _oglRender->BuildModel(*_model);

    wglMakeCurrent(hDC, NULL);

    return true;
}

void StartPreviewWindow(int width, int height)
{
    if (_previewWnd == NULL)
    {
        RECT clientRect = { 0, 0, width, height };
        CreateThread(NULL, 0, RunPreviewWindow, static_cast<LPVOID>(&clientRect), 0, NULL);

        RECT previewRect = {};
        while (_previewWnd == NULL || previewRect.right == 0)
        {
            Sleep(100);

            if (_previewWnd != NULL)
                GetClientRect(_previewWnd, &previewRect);
        }

        InitGL();
        InvalidateRect(_previewWnd, NULL, TRUE);
    }
    else if (width > 0 && height > 0)
    {
        RECT clientRect = { 0, 0, width, height };
        AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(_previewWnd, NULL, 0, 0, clientRect.right, clientRect.bottom, SWP_NOMOVE | SWP_NOOWNERZORDER);
    }
}

void CheckArgs(const vector<string>& args, size_t minCount, const string& error, bool helpActive, const string& help)
{
    if (helpActive)
        throw parse_exception("", help);
    if (args.size() < minCount)
        throw parse_exception(error, help);
}

template <typename Out>
void split(const string& s, char delim, Out result) {
    istringstream iss(s);
    string item;
    while (getline(iss, item, delim)) {
        *result++ = item;
    }
}

vector<string> split(const std::string& s, char delim) {
    vector<string> elems;
    split(s, delim, back_inserter(elems));
    return elems;
}

struct WORKER_THREAD_DATA
{
    CModel* pmodel;
    int nthread;
};

DWORD WINAPI WorkerThreadProc(LPVOID lpParam)
{
    WORKER_THREAD_DATA* pdata = (WORKER_THREAD_DATA*)lpParam;
    pdata->pmodel->DoRender(pdata->nthread);
    delete pdata;
    return 1;
}

#define USE_MULTI_CORE

int _maxThreadCount = 8;

static int GetThreadCount()
{
#ifdef USE_MULTI_CORE
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    int nthreads = (int)sysinfo.dwNumberOfProcessors;
    if (nthreads < 1)
        return 1;
    return min(nthreads, _maxThreadCount);
#else
    return 1;
#endif // USE_MULTI_CORE
}

static void ExecuteMultiCoreRender(CModel& model)
{
    int nthreads = GetThreadCount();
    if (nthreads > 1)
    {
        model.PreRender(nthreads);

        HANDLE* hThreads = new HANDLE[nthreads];
        for (int i = 0; i < nthreads; i++)
        {
            WORKER_THREAD_DATA* pdata = new WORKER_THREAD_DATA;
            pdata->pmodel = &model;
            pdata->nthread = i;
            hThreads[i] = CreateThread(NULL, 0, WorkerThreadProc, (LPVOID)pdata, 0, NULL);
        }
        ::WaitForMultipleObjects(nthreads, hThreads, TRUE, INFINITE);

        model.PostRender(true);
    }
    else
        model.DoRenderSimple();
}

static void RunToFrame(int frame, int frames)
{
    // validate parameters
    if (!_animManager->HasRecords())
        throw parse_exception("No animation loaded");
    if (frames <= 0 && !_animManager->InPlayback())
        throw parse_exception("Frame count must be specified and be greater than 0");
    if (frame > frames)
        throw parse_exception("Frame must be less than frame count");

    if (!_animManager->InPlayback())
        _animManager->StartPlayback(*_model, frames);
    _animManager->GoToFrame(frame);

    _rendBitmap.release();
    if (_previewWnd != NULL)
        InvalidateRect(_previewWnd, NULL, TRUE);
}

static void DoAnimation(const string& path, int width, int height, int frames)
{
    // validate parameters
    if (!_animManager->HasRecords())
        throw parse_exception("No animation loaded");
    if (width == 0)
        throw parse_exception("Width must be specified and be greater than 0");
    if (height == 0)
        throw parse_exception("Height must be specified and be greater than 0");
    if (frames == 0)
        throw parse_exception("Frame count must be specified and be greater than 0");

    if (_animManager->InPlayback())
        _animManager->FinishPlayback();

    _animManager->StartPlayback(*_model, frames);
    for (int frame = 1; frame < frames + 1; frame++)
    {
        _animManager->GoToFrame(frame);

        char file[MAX_PATH];
        sprintf_s(file, 80, "%s\\frame%03d.bmp", CEnv::GetEnv().ExpandPathVars(path).c_str(), frame);

        std::ifstream f(file);
        if (!f.good())
        {
            CBMPTarget bmp;
            bmp.Init(width, height, 24, file);

            CBufferedTarget buffer;
            buffer.SetTarget(static_cast<CRenderTarget*>(&bmp));

            _model->SetRenderTarget(static_cast<CRenderTarget*>(&buffer));
            ExecuteMultiCoreRender(*_model);

            cout << "Frame " << std::to_string(frame) << " complete.\n";
        }
        else
        {
            cout << "Frame " << std::to_string(frame) << " exists, skipping.\n";
        }
    }
    _animManager->FinishPlayback();
}

class CProgressTarget : public CPassThroughTarget
{
protected:
    int _cur, _height, _lastPercent;

public:
    CProgressTarget(CRenderTarget* ptarget) : CPassThroughTarget(ptarget), _cur(0), _lastPercent(0)
    {
        _height = ptarget->GetRenderInfo().height;
    }

    virtual void PreRender(int nthreads)
    {
        _cur = _lastPercent = 0;
        CPassThroughTarget::PreRender(nthreads);
    }

    virtual bool DoLine(int y, const dword* pline)
    {
        int percent = 100 * (++_cur) / _height;
        if (percent != _lastPercent)
        {
            _lastPercent = percent;
            cout << "\r" << std::to_string(_lastPercent) << "% complete...";
        }
        return CPassThroughTarget::DoLine(y, pline);
    }

    virtual void PostRender(bool success)
    {
        cout << "\rFinished                \n";
        CPassThroughTarget::PostRender(success);
    }
};

static void TrackRender(CRenderTarget* ptarget)
{
    CProgressTarget progress(ptarget);
    _model->SetRenderTarget(&progress);
    _model->DoRenderSimple();
}

static bool ParseRunnerCommandString(const std::string& line)
{
    vector<string> items = split(line, ' ');

    bool helpActive = false;
    string command = items[0];
    if (command == "help")
    {
        helpActive = true;

        items.erase(items.begin());
        if (items.empty())
            return false;
        command = items[0];
        if (command == "runner")
            throw parse_exception("", "Runner commands: init, frame, render, play, quit\n");
    }

    if (command == "init")
    {
        CheckArgs(items, 0, "", helpActive,
            "Usage: init\nInitializes the model and animation\n");
        InitParser();
        return true;
    }
    else if (command == "frame")
    {
        CheckArgs(items, 2, "Too few arguments", items.size() < 2 ? helpActive : false,
            "Usage: frame [frame] [frames]\nFast forwards to a given frame\n");
        int frame = stoi(items[1]);
        int frames = stoi(items[2]);
        RunToFrame(frame, frames);
        return true;
    }
    else if (command == "render")
    {
        CheckArgs(items, 2, "Too few arguments", items.size() < 2 ? helpActive : false,
            "Usage: render [type] ...\nRenders an image or movie\n [type] can be jpg, jpeg, bmp, png, movie, preview\n");

        string type = items[1];
        if (type == "jpg" || type == "jpeg")
        {
            CheckArgs(items, 6, "Too few arguments", helpActive,
                "Usage: render jpg [path] [width] [height] [quality]\n");
            string path = items[2];
            int width = stoi(items[3]);
            int height = stoi(items[4]);
            int quality = stoi(items[5]);

            CJPEGTarget target;
            target.Init(width, height, quality, path.c_str());
            TrackRender(static_cast<CRenderTarget*>(&target));
        }
        else if (type == "bmp")
        {
            CheckArgs(items, 6, "Too few arguments", helpActive,
                "Usage: render bmp [path] [width] [height] [bits]\n");
            string path = items[2];
            int width = stoi(items[3]);
            int height = stoi(items[4]);
            int bitcount = stoi(items[5]);

            CJPEGTarget target;
            target.Init(width, height, bitcount, path.c_str());
            TrackRender(static_cast<CRenderTarget*>(&target));
        }
        else if (type == "png")
        {
            CheckArgs(items, 5, "Too few arguments", helpActive,
                "Usage: render png [path] [width] [height]\n");
            string path = items[2];
            int width = stoi(items[3]);
            int height = stoi(items[4]);

            CPNGTarget target;
            target.Init(width, height, path.c_str());
            TrackRender(static_cast<CRenderTarget*>(&target));
        }
        else if (type == "movie")
        {
            // only path is required, unless the script doesn't specify width/height/frames
            CheckArgs(items, 2, "Too few arguments", helpActive,
                "Usage: render movie [path] [width] [height] [frames]\n Note that the script may specify width/height/frames\n");
            string path = items[2];
            int width = stoi(items[3]);
            int height = stoi(items[4]);
            int frames = stoi(items[5]);

            DoAnimation(path, width, height, frames);
        }
        else if (type == "preview")
        {
            CheckArgs(items, 0, "", helpActive,
                "Usage: render preview [width] [height]\n");
            int width = (items.size() > 2 ? stoi(items[2]) : 0);
            int height = (items.size() > 3 ? stoi(items[3]) : 0);

            StartPreviewWindow(width, height);

            unique_ptr<CRendBitmap> rendBitmap = make_unique<CRendBitmap>();

            RECT rect;
            GetClientRect(_previewWnd, &rect);

            HDC hDC = GetDC(_previewWnd);
            (*rendBitmap).Init(rect.right, rect.bottom, hDC);
            ReleaseDC(_previewWnd, hDC);

            TrackRender(static_cast<CRenderTarget*>(rendBitmap.get()));

            _rendBitmap.swap(rendBitmap);
            InvalidateRect(_previewWnd, NULL, TRUE);
        }
        else
            throw parse_exception("Unknown render type");

        return true;
    }
    else if (command == "preview")
    {
        CheckArgs(items, 0, "", helpActive,
            "Usage: preview [width] [height]\nShows/hides the preview window\n");
        int width = (items.size() > 1 ? stoi(items[1]) : 1280);
        int height = (items.size() > 2 ? stoi(items[2]) : 960);

        if (_previewWnd == NULL || (width > 0 && height > 0))
            StartPreviewWindow(width, height);
        else
            PostMessage(_previewWnd, WM_CLOSE, 0, 0);

        return true;
    }
    else if (command == "play")
    {
        CheckArgs(items, 1, "", helpActive,
            "Usage: play [seconds]\nPlays the animation in the preview window\n");
        unsigned int seconds = (items.size() > 1 ? stoi(items[1]) : 2);
        if (seconds < 1)
            throw parse_exception("Preview must be at least 1 second");
        if (seconds > 10)
            throw parse_exception("Preview cannot be more than 10 seconds");

        if (_previewWnd == NULL)
            throw parse_exception("Show the preview window first");
        if (!_animManager->HasRecords())
            throw parse_exception("No animation loaded");
        if (_hGLContext == NULL || _oglRender == nullptr)
            throw parse_exception("No OpenGL rendering context");

        HDC hDC = GetDC(_previewWnd);
        wglMakeCurrent(hDC, _hGLContext);

        int frames = 1000;
        _animManager->StartPlayback(*_model, frames);

        DWORD dwStartMilli = GetTickCount();
        DWORD dwCurrMilli = dwStartMilli;
        do
        {
            dwCurrMilli = GetTickCount();
            double currSeconds = (dwCurrMilli - dwStartMilli) / 1000.0;
            _animManager->GoToFrame(1 + (int) ((frames - 1)*currSeconds / seconds));

            _oglRender->Render(*_model);

            SwapBuffers(hDC);
            //::Sleep(10);
        } while (dwCurrMilli - dwStartMilli < 1000 * seconds);

        _animManager->FinishPlayback();
        wglMakeCurrent(hDC, NULL);

        return true;
    }
    else if (command == "quit")
    {
        CheckArgs(items, 0, "", helpActive, "Usage: quit\nQuits the script runner\n");
        // if no help, shouldn't have made it this far
    }

    return false;
}

static string GetTitleFromPath(const string& path)
{
    size_t slash = path.rfind('\\');
    if (slash == -1)
        return path;
    return path.substr(slash + 1);
}

static string FormatError(const string& script, int line, const string& error)
{
    string prefix;
    if (!script.empty() && line > 0)
        prefix = "[" + GetTitleFromPath(script) + "::" + to_string(line) + "] ";
    return prefix + error;
}

static void ExecuteCommandString(const string& line)
{
    try
    {
        if (!ParseRunnerCommandString(line))
        {
            _parser->ParseCommandString(line);

            _rendBitmap.release();

            if (_previewWnd != NULL)
            {
                if (_oglRender != nullptr)
                {
                    _oglRender->ResetModel();

                    HDC hDC = GetDC(_previewWnd);
                    wglMakeCurrent(hDC, _hGLContext);
                    _oglRender->BuildModel(*_model);
                    wglMakeCurrent(hDC, NULL);
                }
                InvalidateRect(_previewWnd, NULL, TRUE);
            }
        }
    }
    catch (const parse_exception& e)
    {
        string msg = e.what();
        if (!msg.empty())
        {
            cout << FormatError(e.script(), e.line(), msg) << "\n";
            if (!e.help().empty())
                cout << "\n" << e.help();
        }
        else if (!e.help().empty())
            cout << e.help();
    }
    catch (const mig_exception& e)
    {
        cout << e.what() << "\n";
    }
    catch (const invalid_argument& e)
    {
        cout << e.what() << "\n";
    }
}

int main(int argc, char* argv[])
{
    if (!InitParser())
    {
        cout << "Parser failed to initialize, quitting\n";
        return 0;
    }
    cout << "Welcome to the MigRender Script Runner\n Type 'help runner' for runner help, or 'help' for parser help\n";

    for (int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        if (arg == "-h")
        {
            cout << "\nUsage: runner [-h] [-d \"key=path\"] [-mt max_thread_count] [-s command]\n \
                \nOptions:\n \
                -d\tspecifies an environment path using key/value\n \
                -mt\tspecifies maximum thread count during long renders\n \
                -s\texecutes a script command\n";
        }
        else if (arg == "-d" && ++i < argc)
        {
            string keyDir = argv[i];
            size_t equal_pos = keyDir.find('=');
            if (equal_pos != string::npos)
            {
                string key = keyDir.substr(0, equal_pos);
                string dir = keyDir.substr(equal_pos + 1, keyDir.length() - 1);
                if (key == "HOME")
                    CEnv::GetEnv().SetHomePath(dir);
                else
                    CEnv::GetEnv().AddPathVar(key, dir);
            }
        }
        else if (arg == "-mt" && ++i < argc)
        {
            int newMaxThreadCount = atoi(argv[i]);
            _maxThreadCount = max(newMaxThreadCount, 1);
            cout << " Maximum thread count set to " << _maxThreadCount << "\n";
        }
        else if (arg == "-s" && ++i < argc)
        {
            ExecuteCommandString(argv[i]);
        }
    }

    if (argc == 2)
    {
        ExecuteCommandString(argv[1]);
    }

    while (true)
    {
        cout << "> ";

        string line;
        getline(std::cin, line);

        if (line.empty())
            continue;
        if (line == "quit" || line == "exit")
            break;

        ExecuteCommandString(line);
    }

    PostQuitMessage(0);
}
