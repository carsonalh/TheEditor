#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <optional>

#include "FileTree.hpp"
#include "LinearMath.hpp"
#include "D2DRef.hpp"
#include "CommonState.hpp"
#include "FileTreeComponent.hpp"

namespace ed
{

class Application
{
public:
    Application();
    ~Application() = default;

    // Register the window class and call methods for instantiating drawing resources
    HRESULT Initialize();

    // Process and dispatch messages
    void RunMessageLoop();

private:
    // Initialize device-independent resources.
    HRESULT CreateDeviceIndependentResources();

    HRESULT CreateDeviceResources();

    void DiscardDeviceResources();

    HRESULT OnRender();

    void OnResize(
        UINT width,
        UINT height
    );

    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    // windowing and rendering
    HWND hwnd;
    CommonState common_state;

    FileTreeComponent file_tree_component;
    // static constexpr unsigned int MouseDown = 1 << 0;
    // static constexpr unsigned int MousePressed = 1 << 1;

    std::optional<Vec2> mouse_position;
    bool mouse_down;

private:
    FileTree m_FileTree;
};

} // namespace ed
