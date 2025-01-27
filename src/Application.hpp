#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#include <optional>

#include "FileTree.hpp"
#include "LinearMath.hpp"

namespace ed
{

template<typename T>
class D2DRef
{
public:
    D2DRef(T* p = nullptr)
        : m_Resource(p)
    {
        if (p)
        {
            p->AddRef();
        }
    }

    ~D2DRef()
    {
        if (m_Resource)
        {
            m_Resource->Release();
            m_Resource = nullptr;
        }
    }

    D2DRef& operator=(D2DRef&& other)
    {
        m_Resource = other.m_Resource;
        return *this;
    }

    T* operator->() const
    {
        return m_Resource;
    }

    operator T*() const
    {
        return m_Resource;
    }

    bool empty() const
    {
        return m_Resource == nullptr;
    }
private:
    T* m_Resource;
};

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
    HWND m_Hwnd;
    D2DRef<ID2D1Factory> m_Direct2DFactory;
    D2DRef<IDWriteFactory> m_DirectWriteFactory;
    D2DRef<ID2D1HwndRenderTarget> m_RenderTarget;
    D2DRef<ID2D1SolidColorBrush> m_HoverBrush;
    D2DRef<ID2D1SolidColorBrush> m_ActiveBrush;
    D2DRef<ID2D1SolidColorBrush> m_TextBrush;
    D2DRef<IDWriteTextFormat> m_RegularTextFormat;
    D2DRef<IDWriteTextFormat> m_BoldTextFormat;

    static constexpr unsigned int MouseDown = 1 << 0;
    static constexpr unsigned int MousePressed = 1 << 1;

    std::optional<Vec2> m_MousePosition;

private:
    FileTree m_FileTree;
};

} // namespace ed
