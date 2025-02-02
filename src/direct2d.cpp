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
    unsigned index;
} LinearContainerState;

typedef struct
{
    unsigned x, y;
    unsigned width, height;
    unsigned component_index;
    ComponentType component_type;
    union {
        LinearContainerState linear_container;
    } state;
} ParentRect;

typedef struct
{
    int32_t offset_x, offset_y;
    uint32_t length_x, length_y;
} ComputedRect;

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
    unsigned width, height;
    ParentRect *parent_stack;
    int parent_stack_cap;
} RenderDataImpl;

static ComputedRect compute_layout(const RectSpec *layout_spec, const ParentRect *computed_parent);

RenderData *direct2d_init(HWND hwnd)
{
    RenderDataImpl *render_data = (RenderDataImpl*)calloc(1, sizeof *render_data);

    render_data->hwnd = hwnd;

    render_data->parent_stack_cap = 32;
    render_data->parent_stack = (ParentRect*)calloc(
            render_data->parent_stack_cap,
            sizeof *render_data->parent_stack);

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
    free(render_data->parent_stack);
    free(render_data);
}

void direct2d_paint(RenderData *rd, const UiData *ui_data)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;
    render_data->render_target->BeginDraw();
    render_data->render_target->Clear(D2D1::ColorF(D2D1::ColorF::Black));

    ParentRect parent_rect;
    parent_rect.x = 0;
    parent_rect.y = 0;
    parent_rect.width = render_data->width;
    parent_rect.height = render_data->height;
    parent_rect.component_type = COMPONENT_TYPE_BOX_CONTAINER;
    render_data->parent_stack[0] = parent_rect;

    unsigned last_depth = 0;
    int stack_len = 1;

    for (int i = 0; i < ui_data->components_len; i++) {
        unsigned depth = ui_data->components[i].depth;
        if (last_depth >= depth) {
            stack_len -= last_depth - depth + 1;
            assert(stack_len >= 1 && "cannot pop the root transform off the stack");
        } else {
            assert(stack_len < render_data->parent_stack_cap && "stack overflow");
            assert(last_depth == depth - 1 && "child of child with no component in between");
        }
        last_depth = depth;

        const ParentRect *parent = &render_data->parent_stack[stack_len - 1];

        const Component *component = &ui_data->components[i].component;
        const ComputedRect computed = compute_layout((RectSpec*)&component->rect, parent);

        uint32_t color;
        switch (component->type) {
        case COMPONENT_TYPE_BOX_CONTAINER:
            color = component->box_container.color;
            break;
        case COMPONENT_TYPE_RECT:
            color = component->rect.color;
			break;
        default:
			color = 0x00000000;
        }

        memset(&parent_rect, 0, sizeof parent_rect);
        parent_rect.x = computed.offset_x;
        parent_rect.y = computed.offset_y;
        parent_rect.width = computed.length_x;
        parent_rect.height = computed.length_y;
        parent_rect.component_type = component->type;

        render_data->parent_stack[stack_len] = parent_rect;
		stack_len++;

         ID2D1SolidColorBrush *brush;
         HRESULT hr = render_data->render_target->CreateSolidColorBrush(D2D1::ColorF(
             (float)((color >> 24) & 0xFF) / 255.0f,
             (float)((color >> 16) & 0xFF) / 255.0f,
             (float)((color >> 8) & 0xFF) / 255.0f,
             (float)((color >> 0) & 0xFF) / 255.0f
         ), &brush);
         assert(!hr);

         D2D1_RECT_F fill = { 0 };
         fill.left = computed.offset_x;
         fill.top = computed.offset_y;
         fill.right = computed.offset_x + computed.length_x;
         fill.bottom = computed.offset_y + computed.length_y;

         render_data->render_target->FillRectangle(fill, (ID2D1Brush*)brush);
    }

    render_data->render_target->EndDraw();
}

void direct2d_resize(RenderData *rd, unsigned width, unsigned height)
{
    RenderDataImpl *render_data = (RenderDataImpl*)rd;

    render_data->width = width;
    render_data->height = height;
    render_data->render_target->Resize(D2D_SIZE_U {width, height});
}

static ComputedRect compute_layout(const RectSpec *layout_spec, const ParentRect *parent)
{
    ComputedRect computed;

	switch (layout_spec->length_x >> 32) {
	case LENGTH_TYPE_FIXED:
		computed.length_x = (int32_t)(layout_spec->length_x & 0xFFFFFFFF);
		break;
	case LENGTH_TYPE_RELATIVE:
	{
		uint32_t param = layout_spec->length_x & 0xFFFFFFFF;
		float fparam = *(float *)&param;
		computed.length_x = (int)(fparam * parent->width);
	}
		break;
	}

	switch (layout_spec->length_y >> 32) {
	case LENGTH_TYPE_FIXED:
		computed.length_y = (int32_t)(layout_spec->length_y & 0xFFFFFFFF);
		break;
	case LENGTH_TYPE_RELATIVE:
	{
		uint32_t param = layout_spec->length_y & 0xFFFFFFFF;
		float fparam = *(float *)&param;
		computed.length_y = (int)(fparam * parent->height);
	}
		break;
	}

	switch (layout_spec->offset_x >> 32) {
	case OFFSET_TYPE_FIXED_BEGIN:
		computed.offset_x = (int32_t)(layout_spec->offset_x & 0xFFFFFFFF);
		break;
	case OFFSET_TYPE_FIXED_END:
		computed.offset_x = (int32_t)(layout_spec->offset_x & 0xFFFFFFFF) + parent->width - computed.length_x;
		break;
	case OFFSET_TYPE_RELATIVE:
	{
		uint32_t param = layout_spec->offset_x & 0xFFFFFFFF;
		float fparam = *(float *)&param;
		computed.offset_x = (uint32_t)(fparam * (parent->width - computed.length_x));
	}
		break;
	}

	switch (layout_spec->offset_y >> 32) {
	case OFFSET_TYPE_FIXED_BEGIN:
		computed.offset_y = (int32_t)(layout_spec->offset_y & 0xFFFFFFFF);
		break;
	case OFFSET_TYPE_FIXED_END:
		computed.offset_y = (int32_t)(layout_spec->offset_y & 0xFFFFFFFF) + parent->height - computed.length_y;
		break;
	case OFFSET_TYPE_RELATIVE:
	{
		uint32_t param = layout_spec->offset_y & 0xFFFFFFFF;
		float fparam = *(float *)&param;
		computed.offset_y = (uint32_t)(fparam * (parent->height - computed.length_y));
	}
		break;
	}

    return computed;
}

