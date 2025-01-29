#include "the_editor.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

RenderData render_data;

LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

int main(void)
{
    HINSTANCE hinstance = GetModuleHandle(NULL);
    assert(hinstance && "hinstance must be retrieved as not null");

    const TCHAR *class_name = _T("WindowClass");
    WNDCLASS wnd_class = {
        .lpszClassName = class_name,
        .lpfnWndProc = window_proc,
        .style = CS_HREDRAW | CS_VREDRAW,
        .hInstance = hinstance,
    };
    RegisterClass(&wnd_class);

    FileTree filetree;
    filetree_init(&filetree);
    filetree_expand(&filetree, 0);
    for (unsigned i = 0; i < filetree.len; i++) {
        const char *const name = &filetree.strbuffer[filetree.items[i].name_offset];
        const unsigned name_len = filetree.items[i].name_len;
        printf("%.*s\n", name_len, name);
    }
    filetree_uninit(&filetree);

    HWND hwnd = CreateWindow(
            class_name, _T("C Direct2D Window!"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hinstance, NULL);
    assert(hwnd && "hwnd must not be null");

    render_data = (RenderData) { .hwnd = hwnd };
    direct2d_init(&render_data);

    ShowWindow(hwnd, SW_NORMAL);

    MSG message;
    while (GetMessage(&message, hwnd, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}

LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message) {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        direct2d_paint(&render_data);
        ValidateRect(hwnd, NULL);
        break;
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return 0;
}
