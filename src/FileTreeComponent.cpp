#include "FileTreeComponent.hpp"

#include <tchar.h>
#include <cstdlib>
#include <iostream>

using namespace ed;

constexpr float FileTreeLineHeight = 1.25f;

FileTreeComponent::FileTreeComponent(CommonState* state)
    : common_state(state)
{
    auto& direct_write_factory = state->direct_write_factory;
    auto& render_target = state->render_target;

    HRESULT hr;
    IDWriteTextFormat* text_format_ptr;
    tstring font_family_name = _T("Segoe UI");

    hr = direct_write_factory->CreateTextFormat(
        font_family_name.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        32.0f, _T("en-au"), &text_format_ptr
    );
    if (!SUCCEEDED(hr))
    {
        std::cerr << "Failed: create text format\n";
        abort();
    }
    regular_text_format = text_format_ptr;

    hr = direct_write_factory->CreateTextFormat(
        font_family_name.c_str(), nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        32.0f, _T("en-au"), &text_format_ptr
    );
    if (!SUCCEEDED(hr))
    {
        std::cerr << "Failed: create text format\n";
        abort();
    }
    bold_text_format = text_format_ptr;

    ID2D1SolidColorBrush *brush;

    hr = render_target->CreateSolidColorBrush(D2D1::ColorF(0x404040), &brush);
    if (!SUCCEEDED(hr))
    {
        std::cerr << "Failed: create solid color brush\n";
        abort();
    }
    hover_brush = brush;

    hr = render_target->CreateSolidColorBrush(D2D1::ColorF(0x808080), &brush);
    if (!SUCCEEDED(hr))
    {
        std::cerr << "Failed: create solid color brush\n";
        abort();
    }
    active_brush = brush;

    hr = render_target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
    if (!SUCCEEDED(hr))
    {
        std::cerr << "Failed: create solid color brush\n";
        abort();
    }
    text_brush = brush;
}

void FileTreeComponent::update(Size bounds, std::optional<Vec2> mouse_position, bool mouse_down)
{
    auto& file_tree = common_state->file_tree;
    auto& render_target = common_state->render_target;

    const float item_height = FileTreeLineHeight * font_size;

    size_t items_drawn = 0;
    for (int i = 0; i < file_tree.items.size(); i++, items_drawn++)
    {
        D2D1_RECT_F rect = {
            .left = 0.0f,
            .top = (float)(items_drawn * item_height),
            .right = bounds.width,
            .bottom = (float)((items_drawn + 1) * (item_height)),
        };
        const auto& item = file_tree.items[i];

        Rect linear_rect = {
            .x = rect.left,
            .y = rect.top,
            .width = rect.right - rect.left,
            .height = rect.bottom - rect.top,
        };

        if (mouse_position.has_value() && linear_rect.contains(*mouse_position))
        {
            hovered = i;

            if (!is_active() && mouse_down)
            {
                active = i;
                render_target->FillRectangle(rect, active_brush);
            }
            else
            {
                render_target->FillRectangle(rect, hover_brush);
            }
        }

        if (item.flags & FileTreeItem::Directory)
        {
            render_target->DrawText(item.name.c_str(), item.name.length(), bold_text_format, rect, text_brush);

            if (!(item.flags & FileTreeItem::Open))
                while (i + 1 < file_tree.items.size() && file_tree.items[i + 1].depth > item.depth)
                    i++;
        }
        else
        {
            render_target->DrawText(item.name.c_str(), item.name.length(), regular_text_format, rect, text_brush);
        }
    }
}
