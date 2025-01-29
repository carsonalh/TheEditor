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
    TCHAR *tchar_buffer;
    size_t tchar_buffer_cap;
} RenderDataImpl;

RenderData *direct2d_init(HWND hwnd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)calloc(1, sizeof *render_data);

    render_data->hwnd = hwnd;

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

    const float height = 30;
    const float text_height = 20;
    const float text_offset = height - text_height / 2;
    const float padding_left = 8;
    for (unsigned i = 0; i < ui_data->filetree.len; i++) {
        while (ui_data->filetree.items[i].name_len + 1 > render_data->tchar_buffer_cap) {
            size_t cap = render_data->tchar_buffer_cap;
            cap = cap ? cap * 2 : 1024;
            render_data->tchar_buffer = (TCHAR*)realloc((void*)render_data->tchar_buffer, cap * sizeof (TCHAR));
            render_data->tchar_buffer_cap = cap;
        }

		unsigned offset = ui_data->filetree.items[i].name_offset;
        unsigned len = ui_data->filetree.items[i].name_len;
        for (unsigned i = 0; i < len; i++) {
            render_data->tchar_buffer[i] = (TCHAR)ui_data->filetree.strbuffer[offset + i];
        }
        render_data->tchar_buffer[len] = 0;

		D2D1_RECT_F item_rect = { 0, height * i, ui_data->filetree_width, height * (i + 1) };
        D2D1_RECT_F text_rect = item_rect;
        text_rect.left = padding_left;

        float x, y;
        x = (float)ui_data->mouse_x;
        y = (float)ui_data->mouse_y;
        if (item_rect.left < x && x < item_rect.right && item_rect.top < y && y < item_rect.bottom) {
			render_data->render_target->FillRectangle(item_rect, (ID2D1Brush*)render_data->brush_highlight);
        }

        render_data->render_target->DrawTextW(
                render_data->tchar_buffer, len,
                render_data->text_format_regular, text_rect,
				render_data->brush_text);
    }

    render_data->render_target->EndDraw();
}

void direct2d_resize(RenderData *rd, unsigned width, unsigned height)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;

    render_data->render_target->Resize(D2D_SIZE_U {width, height});
}
