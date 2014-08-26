/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "../private.h"
#include "sg/clock.h"
#include "sg/cvar.h"
#include "sg/entry.h"
#include "sg/event.h"
#include "sg/key.h"
#include "sg/opengl.h"
#include "sg/version.h"
#include <Windows.h>
#include <string.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

void
sg_version_platform(struct sg_logger *lp)
{
    (void) lp;
}

static void
quit()
{
    PostQuitMessage(0);
}

void
sg_sys_quit(void)
{
    quit();
}

void
sg_sys_abort(const char *msg)
{
    abort();
}

static struct sg_game_info g_info;
static HDC hDC;
static HGLRC hRC;
/* FIXME: move this to a header file */
HWND sg_window;
static HINSTANCE hInstance;
static int inactive;

static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

static void errorBox(const char *str)
{
    MessageBoxA(NULL, str, "ERROR", MB_OK | MB_ICONEXCLAMATION);
}

static void serrorBox(const char *str)
{
    MessageBoxA(NULL, str, "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
}

static int sg_width, sg_height;

static void killGLWindow()
{
    if (hRC) {
        if (!wglMakeCurrent(NULL, NULL))
            serrorBox("Release of DC and RC failed.");
        if (!wglDeleteContext(hRC))
            serrorBox("Release rendering context failed.");
    }
    hRC = NULL;

    if (hDC && !ReleaseDC(sg_window, hDC))
        serrorBox("Release device context failed.");
    hDC = NULL;

    if (sg_window && !DestroyWindow(sg_window))
        serrorBox("Could not release window.");
    sg_window = NULL;

    if (!UnregisterClassW(L"OpenGL", hInstance))
        serrorBox("Could not unregister class");
    hInstance = NULL;
}

static PIXELFORMATDESCRIPTOR pfd = {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
    PFD_TYPE_RGBA,
    32,
    0, 0, 0, 0, 0, 0, // color bits
    0, // alpha
    0, // shift
    0, 0, 0, 0, 0, // accumulation
    16, // depth buffer
    0, // stencil
    0, // auxiliary
    PFD_MAIN_PLANE, // drawing layer
    0, //reserved
    0, 0, 0 // layer masks
};

BOOL createWindow(int nCmdShow)
{
    GLuint pixelFormat;
    WNDCLASSEXW wc;
    DWORD dwExStyle;
    DWORD dwStyle;
    RECT windowRect;

    windowRect.left = 0;
    windowRect.right = g_info.default_width;
    windowRect.top = 0;
    windowRect.bottom = g_info.default_height;

    hInstance = GetModuleHandle(NULL);
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"OpenGL";
    wc.hIconSm = NULL;

    if (!RegisterClassExW(&wc)) {
        errorBox("Failed to register window class.");
        return FALSE;
    }

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW;
    dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
    sg_window = CreateWindowExW(
        dwExStyle, L"OpenGL", L"Game", dwStyle, 0, 0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL);
    if (!sg_window) {
        killGLWindow();
        errorBox("Window creation error.");
        return FALSE;
    }

    hDC = GetDC(sg_window);
    if (!hDC) {
        killGLWindow();
        errorBox("Can't create OpenGL device context.");
        return FALSE;
    }

    pixelFormat = ChoosePixelFormat(hDC, &pfd);
    if (!pixelFormat) {
        killGLWindow();
        errorBox("Can't find a suitable pixel format.");
        return FALSE;
    }

    if (!SetPixelFormat(hDC, pixelFormat, &pfd)) {
        killGLWindow();
        errorBox("Can't set pixel format.");
        return FALSE;
    }

    hRC = wglCreateContext(hDC);
    if (!hRC) {
        killGLWindow();
        errorBox("Can't create OpenGL rendering context.");
        return FALSE;
    }

    if (!wglMakeCurrent(hDC, hRC)) {
        killGLWindow();
        errorBox("Can't activate OpenGL rendering context.");
        return FALSE;
    }

    ShowWindow(sg_window, nCmdShow);
    SetForegroundWindow(sg_window);
    SetFocus(sg_window);
    sg_width = g_info.default_width;
    sg_height = g_info.default_height;

    return TRUE;
}

static void handleKey(int code, sg_event_type_t t)
{
    struct sg_event_key e;
    int hcode;
    if (code < 0 || code > 255)
        return;
    hcode = SG_WIN_NATIVE_TO_HID[code];
    if (hcode == 255)
        return;
    e.type = t;
    e.key = hcode;
    sg_game_event((union sg_event *) &e);
}

static void handleMouse(int param, sg_event_type_t t, int button)
{
    struct sg_event_mouse e;
    e.type = t;
    e.x = LOWORD(param);
    e.y = sg_height - 1 - HIWORD(param);
    e.button = button;
    sg_game_event((union sg_event *) &e);
}

static LRESULT CALLBACK wndProc(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg) {
    case WM_ACTIVATE:
        inactive = HIWORD(wParam) != 0;
        return 0;

    case WM_SYSCOMMAND:
        switch (wParam) {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            return 0;
        }
        break;

    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    case WM_KEYDOWN:
        handleKey((int) wParam, SG_EVENT_KDOWN);
        return 0;

    case WM_KEYUP:
        handleKey((int) wParam, SG_EVENT_KUP);
        return 0;

    case WM_LBUTTONDOWN:
        handleMouse((int) lParam, SG_EVENT_MDOWN, SG_BUTTON_LEFT);
        return 0;

    case WM_RBUTTONDOWN:
        handleMouse((int) lParam, SG_EVENT_MDOWN, SG_BUTTON_RIGHT);
        return 0;

    case WM_MBUTTONDOWN:
        handleMouse((int) lParam, SG_EVENT_MDOWN, SG_BUTTON_MIDDLE);
        return 0;

    case WM_LBUTTONUP:
        handleMouse((int) lParam, SG_EVENT_MUP, SG_BUTTON_LEFT);
        return 0;

    case WM_RBUTTONUP:
        handleMouse((int) lParam, SG_EVENT_MUP, SG_BUTTON_RIGHT);
        return 0;

    case WM_MBUTTONUP:
        handleMouse((int) lParam, SG_EVENT_MUP, SG_BUTTON_MIDDLE);
        return 0;

    case WM_MOUSEMOVE:
        handleMouse((int) lParam, SG_EVENT_MMOVE, -1);
        return 0;

    case WM_SIZE:
        sg_width = LOWORD(lParam);
        sg_height = HIWORD(lParam);
        return 0;

    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO ifo = (LPMINMAXINFO) lParam;
            RECT wrect, crect;
            int bw, bh;
            GetWindowRect(hWnd, &wrect);
            GetClientRect(hWnd, &crect);
            bw = (wrect.right - wrect.left) - (crect.right - crect.left);
            bh = (wrect.bottom - wrect.top) - (crect.bottom - crect.top);
            ifo->ptMinTrackSize.x = g_info.min_width + bw;
            ifo->ptMinTrackSize.y = g_info.min_height + bh;
            return 0;
        }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

struct cmdline {
    wchar_t *cmd;
    wchar_t **wargv;
    char *arg;
    size_t alloc;
    int argc;
    int idx;
};

static void
cmdline_init(struct cmdline *c)
{
    c->arg = NULL;
    c->alloc = 0;
    c->idx = 1;
    c->cmd = GetCommandLineW();
    if (c->cmd && *c->cmd) {
        c->wargv = CommandLineToArgvW(c->cmd, &c->argc);
        if (!c->wargv)
            abort();
    } else {
        c->wargv = NULL;
        c->argc = 0;
    }
}

static void
cmdline_destroy(struct cmdline *c)
{
    LocalFree(c->wargv);
    free(c->arg);
}

static char *
cmdline_next(struct cmdline *c)
{
    wchar_t *warg;
    size_t wlen;
    int r, len;
    if (c->idx >= c->argc)
        return NULL;
    warg = c->wargv[c->idx++];
    wlen = wcslen(warg);
    if (wlen > INT_MAX)
        return NULL;
    r = WideCharToMultiByte(
        CP_UTF8, 0, warg, (int) wlen, NULL, 0, NULL, NULL);
    if (!r)
        abort();
    len = r;
    if ((unsigned) len + 1 > c->alloc) {
        if (!c->alloc)
            c->alloc = 256;
        while ((unsigned) len + 1 > c->alloc)
            c->alloc *= 2;
        free(c->arg);
        c->arg = malloc(c->alloc);
        if (!c->arg)
            abort();
    }
    r = WideCharToMultiByte(
        CP_UTF8, 0, warg, (int) wlen, c->arg, len, NULL, NULL);
    if (!r)
        abort();
    c->arg[len] = '\0';
    return c->arg;
}

static void
cmdline_parse(void)
{
    struct cmdline c;
    char *arg;
    cmdline_init(&c);
    while ((arg = cmdline_next(&c))) {
        if (arg[0] == '/') {
            switch (arg[1]) {
            case 'D':
                if (!arg[2]) {
                    arg = cmdline_next(&c);
                    if (!arg)
                        goto parseError;
                } else {
                    arg += 2;
                }
                sg_cvar_addarg(NULL, NULL, arg);
                break;

            default:
                goto parseError;
            }
        } else {
            goto parseError;
        }
    }
done:
    cmdline_destroy(&c);
    return;

parseError:
    errorBox("Invalid command line options");
    goto done;
}

static void
init(int nCmdShow)
{
    union sg_event evt;
    HRESULT hr;
    GLenum err;

    cmdline_parse();
    sg_sys_init();
    sg_sys_getinfo(&g_info);

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        errorBox("CoInitializeEx failed");
        exit(1);
    }

    if (!createWindow(nCmdShow))
        exit(0);
    err = glewInit();
    if (err) {
        errorBox("Could not initialize GLEW.");
        exit(1);
    }
    evt.type = SG_EVENT_VIDEO_INIT;
    sg_game_event(&evt);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg;

    init(nCmdShow);

    while(1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                goto done;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!inactive) {
            sg_game_draw(sg_width, sg_height, sg_clock_get());
            SwapBuffers(hDC);
        }
    }
done:

    killGLWindow();
    return 0;
}
