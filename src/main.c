#include "the_editor.h"

#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

static LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

static RenderData *render_data;

int main()
{
    HINSTANCE hinstance = GetModuleHandleW(NULL);
    assert(hinstance && "hinstance must be retrieved as not null");

    const TCHAR *class_name = L"WindowClass";
    WNDCLASSW wnd_class = {
        .lpszClassName = class_name,
        .lpfnWndProc = window_proc,
        .style = CS_HREDRAW | CS_VREDRAW,
        .hInstance = hinstance,
    };
    RegisterClassW(&wnd_class);

    HWND hwnd = CreateWindowW(
            class_name, L"C Direct2D Window!",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hinstance, NULL);
    assert(hwnd && "hwnd must not be null");

    render_data = direct2d_init(hwnd);

    ShowWindow(hwnd, SW_NORMAL);

    MSG message;
    while (GetMessageW(&message, hwnd, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    direct2d_uninit(render_data);
}

static LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
		{
			RECT client;
            GetClientRect(hwnd, &client);
            const unsigned width = client.right - client.left;
            const unsigned height = client.bottom - client.top;
			direct2d_resize(render_data, width, height);
		}
        break;
    case WM_KEYDOWN:
        {
            switch (wparam) {
            case VK_LEFT:
                break;
            case VK_RIGHT:
                break;
            }
        }
        break;
    case WM_MOUSEMOVE:
        InvalidateRect(hwnd, NULL, false);
        break;
    case WM_PAINT:
        direct2d_paint(render_data);
        ValidateRect(hwnd, NULL);
        break;
    default:
        return DefWindowProcW(hwnd, message, wparam, lparam);
    }

    return 0;
}
