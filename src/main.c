#include "direct2d.h"
#include "neovim.h"

#include <synchapi.h>
#include <windows.h>
#include <windowsx.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

static LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

static RenderData *render_data;

int main(void)
{
    HINSTANCE hinstance = GetModuleHandleW(NULL);
    assert(hinstance && "hinstance must be retrieved as not null");

    DWORD neovim_thread_id;
    HANDLE neovim_thread = CreateThread(NULL, 0, neovim_thread_main, NULL, 0, &neovim_thread_id);
    assert(neovim_thread);

    const wchar_t *class_name = L"WindowClass";
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

    DWORD result;
    result = WaitForSingleObject(neovim_thread, INFINITE);
    assert(!result);

    BOOL ok;
    ok = CloseHandle(neovim_thread);
    assert(ok && "destroy neovim thread");
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
