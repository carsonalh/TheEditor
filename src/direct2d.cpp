#include "the_editor.h"

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

typedef struct
{
    ID2D1Factory *factory;
    ID2D1HwndRenderTarget *render_target;
    ID2D1SolidColorBrush *brush_highlight;
    ID2D1SolidColorBrush *brush_active;
    ID2D1SolidColorBrush *brush_text;
    IDWriteFactory *dwrite_factory;
    IDWriteTextFormat *text_format_regular;
    IDWriteTextFormat *text_format_bold;
    HWND hwnd;
    int (*offset_stack)[2];
    int offset_stack_cap;
} RenderDataImpl;

RenderData *direct2d_init(HWND hwnd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)calloc(1, sizeof *render_data);

    render_data->hwnd = hwnd;

    render_data->offset_stack_cap = 32;
    render_data->offset_stack = (int(*)[2])calloc(
            render_data->offset_stack_cap,
            sizeof *render_data->offset_stack);

    HRESULT hr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &render_data->factory);
    assert(!hr && "create d2d1 factory");

    RECT rect;
    GetClientRect(hwnd, &rect);

    D2D1_SIZE_U size = D2D1::SizeU(
        rect.right - rect.left,
        rect.bottom - rect.top
    );

    hr = render_data->factory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(hwnd, size),
            &render_data->render_target);
    assert(!hr && "create hwnd render target");

    hr = render_data->render_target->CreateSolidColorBrush(
            D2D1::ColorF(0x303030),
            &render_data->brush_highlight);
    assert(!hr && "create highlight brush");

    hr = render_data->render_target->CreateSolidColorBrush(
            D2D1::ColorF(0x606060),
            &render_data->brush_active);
    assert(!hr && "create active brush");

    hr = render_data->render_target->CreateSolidColorBrush(
            D2D1::ColorF(0xFFFFFF),
            &render_data->brush_text);
    assert(!hr && "create text brush");

    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED,
            __uuidof(IDWriteFactory),
			(IUnknown**)&render_data->dwrite_factory);
    assert(!hr && "create DirectWrite factory");

    hr = render_data->dwrite_factory->CreateTextFormat(
			_T("Segoe UI"), NULL, DWRITE_FONT_WEIGHT_LIGHT,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			20.0f, _T("en-au"), &render_data->text_format_regular);
    assert(!hr && "create regular text format");

    hr = render_data->dwrite_factory->CreateTextFormat(
			_T("Segoe UI"), NULL, DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			20.0f, _T("en-au"), &render_data->text_format_bold);
    assert(!hr && "create regular text format");

    return render_data;
}

void direct2d_uninit(RenderData *rd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;
    render_data->factory->Release();
    render_data->render_target->Release();
    free(render_data);
}

void direct2d_paint(RenderData *rd, const UiData *ui_data)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;
    render_data->render_target->BeginDraw();
    render_data->render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));

	render_data->offset_stack[0][0] = 0;
	render_data->offset_stack[0][1] = 0;
    unsigned last_depth = 0;
    int stack_len = 1;

    for (int i = 0; i < ui_data->components_len; i++) {
        unsigned depth = ui_data->components[i].depth;
        if (last_depth >= depth) {
            stack_len -= last_depth - depth + 1;
            assert(stack_len >= 1 && "cannot pop the root transform off the stack");
        } else {
            assert(stack_len < render_data->offset_stack_cap && "stack overflow");
            assert(last_depth == depth - 1 && "child of child with no component in between");
        }
        last_depth = depth;

        int offset_x = render_data->offset_stack[stack_len - 1][0];
        int offset_y = render_data->offset_stack[stack_len - 1][1];

		render_data->offset_stack[stack_len][0] = offset_x + ui_data->components[i].offset_x;
		render_data->offset_stack[stack_len][1] = offset_y + ui_data->components[i].offset_y;
		stack_len++;

        uint32_t color = ui_data->components[i].color;
        ID2D1SolidColorBrush *brush;
        HRESULT hr = render_data->render_target->CreateSolidColorBrush(D2D1::ColorF(
            (float)((color >> 24) & 0xFF) / 255.0f,
            (float)((color >> 16) & 0xFF) / 255.0f,
            (float)((color >> 8) & 0xFF) / 255.0f,
            (float)((color >> 0) & 0xFF) / 255.0f
        ), &brush);
        assert(!hr);

        render_data->render_target->FillRectangle(D2D1_RECT_F {
            (float)(offset_x + ui_data->components[i].offset_x),
            (float)(offset_y + ui_data->components[i].offset_y),
            (float)(offset_x + ui_data->components[i].offset_x + ui_data->components[i].width),
            (float)(offset_y + ui_data->components[i].offset_y + ui_data->components[i].height),
        }, (ID2D1Brush*)brush);
    }

    render_data->render_target->EndDraw();
}

void direct2d_resize(RenderData *rd, unsigned width, unsigned height)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;

    render_data->render_target->Resize(D2D_SIZE_U {width, height});
}
