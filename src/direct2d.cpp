#include "the_editor.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void direct2d_init(RenderData *render_data)
{
    HRESULT hr;
    HWND hwnd = render_data->hwnd;

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
}

void direct2d_uninit(RenderData *render_data)
{
    render_data->factory->Release();
    render_data->render_target->Release();
}

void direct2d_paint(RenderData *render_data)
{
    render_data->render_target->BeginDraw();
    render_data->render_target->Clear(D2D1::ColorF(D2D1::ColorF::Beige));
    render_data->render_target->EndDraw();
}
