#include "client/keyboard/keyid.h"
#include "client/keyboard/keytable.h"
#include "client/ui/event.hpp"
#include "client/ui/menu.hpp"
#include "client/ui/window.hpp"
#include "sys/path.hpp"
#include "sys/rand.hpp"
#include <windows.h>
#include <gl\gl.h>
#include <gl\glu.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

class WinWindow : public UI::Window {
public:
    virtual void close();
};

void WinWindow::close()
{
    PostQuitMessage(0);
}

static HDC hDC;
static HGLRC hRC;
static HWND hWnd;
static HINSTANCE hInstance;

static bool inactive;

static WinWindow *gWindow;

static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

static void errorBox(const char *str)
{
    MessageBox(NULL, str, "ERROR", MB_OK | MB_ICONEXCLAMATION);
}

static void serrorBox(const char *str)
{
    MessageBox(NULL, str, "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
}

static void handleResize(int width, int height)
{
    if (!height)
        height = 1;
    if (!width)
        width = 1;
    glViewport(0, 0, width, height);
    gWindow->setSize(width, height);
}

static void killGLWindow()
{
    if (hRC) {
        if (!wglMakeCurrent(NULL, NULL))
            serrorBox("Release of DC and RC failed.");
        if (!wglDeleteContext(hRC))
            serrorBox("Release rendering context failed.");
    }
    hRC = NULL;

    if (hDC && !ReleaseDC(hWnd, hDC))
        serrorBox("Release device context failed.");
    hDC = NULL;

    if (hWnd && !DestroyWindow(hWnd))
        serrorBox("Could not release window.");
    hWnd = NULL;

    if (!UnregisterClass("OpenGL", hInstance))
        serrorBox("Could not unregister class");
    hInstance = NULL;
}

BOOL createWindow(const char *title, int width, int height)
{
    GLuint pixelFormat;
    WNDCLASS wc;
    DWORD dwExStyle;
    DWORD dwStyle;
    RECT windowRect;

    windowRect.left = 0;
    windowRect.right = width;
    windowRect.top = 0;
    windowRect.bottom = height;

    hInstance = GetModuleHandle(NULL);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = (WNDPROC) wndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "OpenGL";

    if (!RegisterClass(&wc)) {
        errorBox("Failed to register window class.");
        return FALSE;
    }

    dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
    dwStyle = WS_OVERLAPPEDWINDOW;
    dwStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

    AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);
    hWnd = CreateWindowEx(dwExStyle, "OpenGL", title, dwStyle, 0, 0,
                          windowRect.right - windowRect.left,
                          windowRect.bottom - windowRect.top,
                          NULL, NULL, hInstance, NULL);
    if (!hWnd) {
        killGLWindow();
        errorBox("Window creation error.");
        return FALSE;
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

    hDC = GetDC(hWnd);
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

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);
    handleResize(width, height);

    return TRUE;
}

static void handleKey(int code, UI::EventType t)
{
	if (code < 0 || code > 255)
		return;
	int hcode = WIN_NATIVE_TO_HID[code];
	if (hcode == 255)
		return;
	char buf[64];
	_snprintf(buf, sizeof(buf), "key: %s\n", keyid_name_from_code(hcode));
	buf[63] = '\0';
	OutputDebugString(buf);
	UI::KeyEvent e(t, hcode);
	gWindow->handleEvent(e);
}

static void handleMouse(int param, UI::EventType t, int button)
{
	int x = LOWORD(param), y = HIWORD(param);
	UI::MouseEvent e(t, button, x, gWindow->height() - 1 - y);
	gWindow->handleEvent(e);
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		handleKey(wParam, UI::KeyDown);
		return 0;

    case WM_KEYUP:
		handleKey(wParam, UI::KeyUp);
        return 0;

	case WM_LBUTTONDOWN:
		handleMouse(lParam, UI::MouseDown, UI::ButtonLeft);
		return 0;

	case WM_RBUTTONDOWN:
		handleMouse(lParam, UI::MouseDown, UI::ButtonRight);
		return 0;

	case WM_MBUTTONDOWN:
		handleMouse(lParam, UI::MouseDown, UI::ButtonMiddle);
		return 0;

	case WM_LBUTTONUP:
		handleMouse(lParam, UI::MouseUp, UI::ButtonLeft);
		return 0;

	case WM_RBUTTONUP:
		handleMouse(lParam, UI::MouseUp, UI::ButtonRight);
		return 0;

	case WM_MBUTTONUP:
		handleMouse(lParam, UI::MouseUp, UI::ButtonMiddle);
		return 0;

	case WM_MOUSEMOVE:
		handleMouse(lParam, UI::MouseMove, -1);
		return 0;

    case WM_SIZE:
        handleResize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int iCmdShow)
{
    MSG msg;
    WinWindow w;
    gWindow = &w;

    Path::init();
    Rand::global.seed();
    w.setScreen(new UI::Menu);

    if (!createWindow("Game", 768, 480))
        return 0;

    while (1) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                goto done;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!inactive) {
            w.draw();
            SwapBuffers(hDC);
        }
    }
done:
    
    killGLWindow();
    return 0;
}
