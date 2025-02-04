#include "direct2d.h"

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <tchar.h>

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <string.h>

typedef struct
{
    ID2D1Factory *factory;
    ID2D1HwndRenderTarget *render_target;
    ID2D1SolidColorBrush *brush_highlight;
    ID2D1SolidColorBrush *brush_active;
    ID2D1SolidColorBrush *brush_text;
    IDWriteFactory *dwrite_factory;
    IDWriteTextFormat *text_format;
    IDWriteTextLayout *text_layout;
    HWND hwnd;
    unsigned width, height;
} RenderDataImpl;

const wchar_t lorem_ipsum[] = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam sit amet sagittis magna. Pellentesque eu quam volutpat purus egestas tincidunt. Cras tristique egestas magna, et convallis odio blandit et. Sed dictum justo vitae velit elementum elementum. Curabitur sit amet vestibulum sem. Aenean convallis, turpis a viverra lobortis, orci tellus maximus arcu, at rhoncus nisi lacus ac dolor. In hac habitasse platea dictumst. Fusce eu condimentum sapien. Integer vitae pretium arcu. Donec at nisl orci. Etiam tempor tempor nibh, non dictum neque egestas consectetur. Praesent a tristique erat. Praesent blandit sodales dolor, non commodo ligula varius et. Sed consectetur maximus massa, quis gravida lorem malesuada tristique. Curabitur tempus nec leo quis feugiat.\n\n"
"Morbi vestibulum laoreet urna a gravida. Etiam blandit pharetra nisl, nec fringilla lectus tempus sed. Duis eget scelerisque risus. Curabitur massa enim, luctus et odio ut, mattis finibus enim. Etiam lacinia felis ac gravida dapibus. Maecenas rhoncus facilisis orci, in tempor lacus sollicitudin non. Suspendisse blandit arcu a faucibus varius. Aenean ultricies ullamcorper hendrerit. Nulla facilisi. Praesent auctor sed sem at hendrerit. Praesent a turpis aliquet, venenatis eros eu, semper mi. Fusce ut accumsan sem, in malesuada elit. Donec quis quam eu justo ullamcorper facilisis. Nullam bibendum urna at.";

RenderData *direct2d_init(HWND hwnd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)calloc(1, sizeof *render_data);

    render_data->hwnd = hwnd;

    HRESULT hr;
    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &render_data->factory);
    assert(!hr && "create d2d1 factory");

    RECT rect;
    GetClientRect(hwnd, &rect);

    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;

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
			L"Segoe UI", NULL, DWRITE_FONT_WEIGHT_LIGHT,
			DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
			20.0f, L"en-au", &render_data->text_format);
    assert(!hr && "create text format");

    hr = render_data->dwrite_factory->CreateTextLayout(
            lorem_ipsum, wcslen(lorem_ipsum), render_data->text_format,
            (float)width, (float)height, &render_data->text_layout);
    assert(!hr && "create text layout");

    return render_data;
}

void direct2d_uninit(RenderData *rd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;
    render_data->factory->Release();
    render_data->render_target->Release();
    free(render_data);
}

void direct2d_paint(RenderData *rd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;
    render_data->render_target->BeginDraw();
    render_data->render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    render_data->render_target->DrawTextLayout(D2D1::Point2F(), render_data->text_layout, render_data->brush_text);

    render_data->render_target->EndDraw();
}

void direct2d_resize(RenderData *rd, unsigned width, unsigned height)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;

    render_data->width = width;
    render_data->height = height;
    render_data->render_target->Resize(D2D_SIZE_U {width, height});

    render_data->text_layout->Release();
    HRESULT hr = render_data->dwrite_factory->CreateTextLayout(
            lorem_ipsum, wcslen(lorem_ipsum), render_data->text_format,
            (float)width, (float)height, &render_data->text_layout);
    assert(!hr && "re-create text layout on resize");
}
