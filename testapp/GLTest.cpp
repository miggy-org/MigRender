#include "StdAfx.h"
#include <fstream>

#include "GLTest.h"
#include "gl/oglrender.h"

/*
    See here for a good tutorial and reference: https://learnopengl.com/
*/

using namespace MigRender;
using namespace std;

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

bool GLTest(CModel& model, CAnimManager& animManager, HWND hWnd, int frames)
{
    HDC hDC = ::GetDC(hWnd);
    if (!InitPixelFormat(hDC))
        return false;

    HGLRC hContext = wglCreateContext(hDC);
    if (hContext == NULL)
    {
        DWORD err = GetLastError();
        return false;
    }
    wglMakeCurrent(hDC, hContext);

    OutputGLString("Version: ", GL_VERSION);
    OutputGLString("Vendor: ", GL_VENDOR);
    OutputGLString("Renderer: ", GL_RENDERER);

    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    glViewport(0, 0, clientRect.right, clientRect.bottom);

    OglRender oglRender;
    oglRender.Init();

    oglRender.GetProgram(OglProgramType::Main)->BeginProgram()
        .AddShader(OglShader(GL_VERTEX_SHADER, ReadShader("test.vs")))
        .AddShader(OglShader(GL_FRAGMENT_SHADER, ReadShader("test.fs")))
        .LinkProgram();

    oglRender.GetProgram(OglProgramType::Bg)->BeginProgram()
        .AddShader(OglShader(GL_VERTEX_SHADER, ReadShader("bg.vs")))
        .AddShader(OglShader(GL_FRAGMENT_SHADER, ReadShader("bg.fs")))
        .LinkProgram();

    oglRender.BuildModel(model);

    if (animManager.HasRecords())
    {
        animManager.StartPlayback(model, frames);
        for (int frame = 1; frame <= frames; frame++)
        {
            animManager.GoToFrame(frame);

            oglRender.Render(model);

            SwapBuffers(hDC);
            //::Sleep(10);
        }
        animManager.FinishPlayback();
    }
    else
    {
        oglRender.Render(model);
        SwapBuffers(hDC);
    }

    oglRender.Term();

    wglMakeCurrent(hDC, NULL);
    wglDeleteContext(hContext);

    return true;
}
