#pragma once

#include "FileTree.hpp"
#include "D2DRef.hpp"

#include <d2d1.h>
#include <dwrite.h>

namespace ed
{

struct CommonState
{
    FileTree file_tree;

    D2DRef<ID2D1Factory> direct2d_factory;
    D2DRef<IDWriteFactory> direct_write_factory;
    D2DRef<ID2D1HwndRenderTarget> render_target;

public:
    CommonState();
};

}
