#ifdef UNICODE
#undef UNICODE
#endif // UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>

#include <memory>
#include <iostream>

#include "FileTree.hpp"

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

class DemoApp
{
public:
    DemoApp();
    ~DemoApp() = default;

    // Register the window class and call methods for instantiating drawing resources
    HRESULT Initialize();

    // Process and dispatch messages
    void RunMessageLoop();

private:
    // Initialize device-independent resources.
    HRESULT CreateDeviceIndependentResources();

    // Initialize device-dependent resources.
    HRESULT CreateDeviceResources();

    // Release device-dependent resource.
    void DiscardDeviceResources();

    // Draw content.
    HRESULT OnRender();

    // Resize the render target.
    void OnResize(
        UINT width,
        UINT height
    );

    // The windows procedure.
    static LRESULT CALLBACK WndProc(
        HWND hWnd,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
    );

private:
    HWND m_hwnd;
    D2DResource<ID2D1Factory> m_pDirect2dFactory;
    D2DResource<ID2D1HwndRenderTarget> m_RenderTarget;
    D2DResource<ID2D1SolidColorBrush> m_LightSlateGrayBrush;
    D2DResource<ID2D1SolidColorBrush> m_CornflowerBlueBrush;
};

DemoApp::DemoApp()
    : m_hwnd(nullptr)
    , m_pDirect2dFactory()
    , m_LightSlateGrayBrush()
    , m_CornflowerBlueBrush()
    , m_RenderTarget()
{
}

HRESULT DemoApp::Initialize()
{
    HRESULT hr;

    hr = CreateDeviceIndependentResources();

    if (!SUCCEEDED(hr))
        return hr;

    HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DemoApp::WndProc;
    wc.cbWndExtra = sizeof (LONG_PTR);
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
    wc.lpszClassName = "Direct2DDemo";
    RegisterClassEx(&wc);

    m_hwnd = CreateWindow(
        "Direct2DDemo",
        "Direct2d Demo Application",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, nullptr, nullptr,
        instance,
        this
    );

    SetWindowPos(m_hwnd, nullptr, 0, 0, 720, 480, SWP_NOMOVE);
    ShowWindow(m_hwnd, SW_SHOWNORMAL);
    UpdateWindow(m_hwnd);

    return hr;
}

void DemoApp::RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_QUIT)
            break;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

HRESULT DemoApp::OnRender()
{
    HRESULT hr;
    hr = CreateDeviceResources();
    if (!SUCCEEDED(hr))
        return hr;

    m_RenderTarget->BeginDraw();
    m_RenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
    m_RenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    auto size = m_RenderTarget->GetSize();

    for (float x = 10; x < size.width; x += 10)
    {
        const auto from = D2D1::Point2(x, 0.0f);
        const auto to = D2D1::Point2(x, size.height);
        m_RenderTarget->DrawLine(from, to, m_LightSlateGrayBrush);
    }

    for (float y = 10; y < size.height; y += 10)
    {
        const auto from = D2D1::Point2(0.0f, y);
        const auto to = D2D1::Point2(size.width, y);
        m_RenderTarget->DrawLine(from, to, m_LightSlateGrayBrush);
    }

    auto rect1 = D2D1::RectF(
        size.width / 2.0f - 50.0f,
        size.height / 2.0f - 50.0f,
        size.width / 2.0f + 50.0f,
        size.height / 2.0f + 50.0f
    );

    auto rect2 = D2D1::RectF(
        size.width / 2.0f - 100.0f,
        size.height / 2.0f - 100.0f,
        size.width / 2.0f + 100.0f,
        size.height / 2.0f + 100.0f
    );

    m_RenderTarget->FillRectangle(rect1, m_LightSlateGrayBrush);
    m_RenderTarget->DrawRectangle(rect2, m_CornflowerBlueBrush);

    hr = m_RenderTarget->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
    {
        fprintf(stderr, "Unhandled: D2DERR_RECREATE_TARGET\n");
        return hr;
    }

    return hr;
}

void DemoApp::OnResize(unsigned int width, unsigned int height)
{
    if (!m_RenderTarget.empty())
        m_RenderTarget->Resize(D2D1::SizeU(width, height));
}

LRESULT DemoApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto app = reinterpret_cast<DemoApp*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        {
            auto cs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            auto da = reinterpret_cast<DemoApp*>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(da));
        }
        return 1;
    case WM_CLOSE:
    case WM_QUIT:
        PostQuitMessage(0);
        break;
    case WM_DESTROY:
        DestroyWindow(hWnd);
        break;
    case WM_SIZE:
        {
            unsigned int width, height;
            width = LOWORD(lParam);
            height = HIWORD(lParam);
            app->OnResize(width, height);
        }
        return 0;
    case WM_DISPLAYCHANGE:
        InvalidateRect(hWnd, nullptr, false);
        return 0;
    case WM_PAINT:
        app->OnRender();
        ValidateRect(hWnd, nullptr);
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

HRESULT DemoApp::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    ID2D1Factory *factory;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    if (!SUCCEEDED(hr))
        return hr;

    m_pDirect2dFactory = factory;

    return hr;
}

HRESULT DemoApp::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    ID2D1HwndRenderTarget *renderTarget;
    ID2D1SolidColorBrush *brush;
    RECT rect;
    D2D1_SIZE_U size;

    if (!m_RenderTarget.empty())
        return S_OK;

    GetClientRect(m_hwnd, &rect);
    size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

    hr = m_pDirect2dFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_hwnd, size),
        &renderTarget
    );
    if (!SUCCEEDED(hr))
        return hr;
    m_RenderTarget = renderTarget;

    hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::LightSlateGray), &brush);
    if (!SUCCEEDED(hr))
        return hr;
    m_LightSlateGrayBrush = brush;

    hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CornflowerBlue), &brush);
    if (!SUCCEEDED(hr))
        return hr;
    m_CornflowerBlueBrush = brush;

    return hr;
}

void DemoApp::DiscardDeviceResources()
{
}

int main(void)
{
#ifndef _NDEBUG
    // Is this necessary with ASAN?
    HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
#endif
    {
        ed::FileTree ft;
    }

    {
        DemoApp app;

        if (!SUCCEEDED(app.Initialize()))
        {
            fprintf(stderr, "failed to initialise the application\n");
            exit(1);
        }

        app.RunMessageLoop();
    }
}
