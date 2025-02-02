#include "the_editor.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>

static LRESULT window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
static void compute_ui_component_tree(void);

static RenderData *render_data;
static UiData *ui_data;

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

    HWND hwnd = CreateWindow(
            class_name, _T("C Direct2D Window!"),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            NULL, NULL, hinstance, NULL);
    assert(hwnd && "hwnd must not be null");

    render_data = direct2d_init(hwnd);

    ui_data = calloc(1, sizeof *ui_data);
    const size_t components_cap = 256;
    ComponentNode *components = malloc(components_cap * sizeof *components);
    *ui_data = (UiData) {
        .hot = -1,
        .active = -1,
        .components = components,
        .components_cap = components_cap,
    };

    ShowWindow(hwnd, SW_NORMAL);

    MSG message;
    while (GetMessage(&message, hwnd, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    free(ui_data);
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
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
        break;
    case WM_MOUSEMOVE:
        ui_data->mouse_x = GET_X_LPARAM(lparam);
        ui_data->mouse_y = GET_Y_LPARAM(lparam);
        InvalidateRect(hwnd, NULL, false);
        break;
    case WM_PAINT:
        compute_ui_component_tree();
        direct2d_paint(render_data, ui_data);
        ValidateRect(hwnd, NULL);
        break;
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return 0;
}

static void compute_ui_component_tree(void)
{
    ui_begin(ui_data);

    //ui_box_container(&(BoxContainer) {
    //    .length_x = LENGTH_RELATIVE(1.0f),
    //    .length_y = LENGTH_RELATIVE(1.0f),
    //    .color = 0xff0000ff,
    //});
    //ui_depth_push();
    //    ui_rect(&(Rect) {
    //        .length_x = LENGTH_FIXED(100),
    //        .length_y = LENGTH_FIXED(100),
    //        .offset_x = OFFSET_RELATIVE(0.5f),
    //        .offset_y = OFFSET_RELATIVE(0.5f),
    //        .color = 0x0000ffff,
    //    });
    //ui_depth_pop();

    ui_box_container(&(BoxContainer) {
        .length_x = LENGTH_RELATIVE(1.0f),
        .length_y = LENGTH_RELATIVE(1.0f),
        .color = 0x404040ff,
    });
    ui_linear_container(&(LinearContainer) {
        .direction = LINEAR_CONTAINER_LEFT_TO_RIGHT,
            .item_length_px = 50,
            .length_x = LENGTH_RELATIVE(1.0f),
            .length_y = LENGTH_RELATIVE(0.5f),
            .offset_x = OFFSET_FROM_BEGIN(0),
            .offset_y = OFFSET_FROM_BEGIN(0),
    });
    ui_depth_push();
		ui_rect(&(Rect) {
            .length_x = LENGTH_RELATIVE(1.0f),
            .length_y = LENGTH_RELATIVE(1.0f),
			.color = 0xff0000ff,
		});
		ui_rect(&(Rect) {
            .length_x = LENGTH_RELATIVE(1.0f),
            .length_y = LENGTH_RELATIVE(1.0f),
			.color = 0x00ff00ff,
		});
		ui_rect(&(Rect) {
            .length_x = LENGTH_RELATIVE(1.0f),
            .length_y = LENGTH_RELATIVE(1.0f),
			.color = 0x0000ffff,
		});
    ui_depth_pop();

    ui_end();
}
