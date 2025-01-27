#pragma once

#include "TheEditor.hpp"
#include "CommonState.hpp"
#include "D2DRef.hpp"
#include "FileTree.hpp"
#include "LinearMath.hpp"

#include <d2d1.h>
#include <dwrite.h>

#include <optional>

namespace ed
{

class FileTreeComponent
{
public:
    // index of the currently hovered element
    int hovered = -1;
    int active = -1;
    // bool hidden;
    float font_size;
    CommonState* common_state;
    D2DRef<ID2D1SolidColorBrush> text_brush;
    D2DRef<ID2D1SolidColorBrush> hover_brush;
    D2DRef<ID2D1SolidColorBrush> active_brush;
    D2DRef<IDWriteTextFormat> regular_text_format;
    D2DRef<IDWriteTextFormat> bold_text_format;

public:
    FileTreeComponent(CommonState *state);

    /* Renders at the origin.  This should be called within a direct2d layer, because it's easier
       to clip draws than to perfectly stay within bounds. */
    void update(Size bounds, std::optional<Vec2> mouse_position, bool mouse_down);

    /* This disallows other elements to handle mouse events until this is released. */
    inline bool is_active() const { return active >= 0; }
};

}
