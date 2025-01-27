#include "Application.hpp"

#include <tchar.h>
#include <iostream>

Application::Application()
    : m_Hwnd(nullptr)
    , m_Direct2DFactory()
    , m_LightSlateGrayBrush()
    , m_CornflowerBlueBrush()
    , m_RenderTarget()
{
}

HRESULT Application::Initialize()
{
    HRESULT hr;

    hr = CreateDeviceIndependentResources();

    if (!SUCCEEDED(hr))
        return hr;

    HINSTANCE instance = GetModuleHandle(nullptr);
    WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = Application::WndProc;
    wc.cbWndExtra = sizeof (LONG_PTR);
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
    wc.lpszClassName = _T("Direct2DDemo");
    RegisterClassEx(&wc);

    m_Hwnd = CreateWindow(
        _T("Direct2DDemo"),
        _T("Direct2d Demo Application"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, nullptr, nullptr,
        instance,
        this
    );

    SetWindowPos(m_Hwnd, nullptr, 0, 0, 720, 480, SWP_NOMOVE);
    ShowWindow(m_Hwnd, SW_SHOWNORMAL);
    UpdateWindow(m_Hwnd);

    return hr;
}

void Application::RunMessageLoop()
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

HRESULT Application::OnRender()
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
        std::cerr <<  "Unhandled: D2DERR_RECREATE_TARGET\n";
        return hr;
    }

    return hr;
}

void Application::OnResize(unsigned int width, unsigned int height)
{
    if (!m_RenderTarget.empty())
        m_RenderTarget->Resize(D2D1::SizeU(width, height));
}

LRESULT Application::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    auto app = reinterpret_cast<Application*>(GetWindowLongPtrA(hWnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
        {
            auto cs = reinterpret_cast<LPCREATESTRUCT>(lParam);
            auto da = reinterpret_cast<Application*>(cs->lpCreateParams);
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

HRESULT Application::CreateDeviceIndependentResources()
{
    HRESULT hr = S_OK;

    ID2D1Factory *factory;

    hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &factory);
    if (!SUCCEEDED(hr))
        return hr;

    m_Direct2DFactory = factory;

    return hr;
}

HRESULT Application::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    ID2D1HwndRenderTarget *renderTarget;
    ID2D1SolidColorBrush *brush;
    RECT rect;
    D2D1_SIZE_U size;

    if (!m_RenderTarget.empty())
        return S_OK;

    GetClientRect(m_Hwnd, &rect);
    size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

    hr = m_Direct2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(m_Hwnd, size),
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

void Application::DiscardDeviceResources()
{
}
