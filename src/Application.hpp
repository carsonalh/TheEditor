#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d2d1.h>

template<typename T>
class D2DResource
{
public:
    D2DResource()
        : m_Resource(nullptr)
    {
    }

    ~D2DResource()
    {
        if (m_Resource)
            m_Resource->Release();

        // a pointer given by direct2d will free itself
    }

    T* operator=(T* p)
    {
        return m_Resource = p;
    }

    T* operator->()
    {
        return m_Resource;
    }

    operator T*()
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
    HWND m_Hwnd;
    D2DResource<ID2D1Factory> m_Direct2DFactory;
    D2DResource<ID2D1HwndRenderTarget> m_RenderTarget;
    D2DResource<ID2D1SolidColorBrush> m_LightSlateGrayBrush;
    D2DResource<ID2D1SolidColorBrush> m_CornflowerBlueBrush;
};
