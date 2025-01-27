#include "Application.hpp"

#include <windowsx.h>
#include <dwrite.h>
#include <tchar.h>
#include <iostream>

using namespace ed;

Application::Application()
    : m_Hwnd(nullptr)
    , m_MousePosition(std::nullopt)
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
    m_RenderTarget->Clear(D2D1::ColorF(0x202020));

    size_t items_drawn = 0;
    constexpr float item_height = 48.0f;

    for (int i = 0; i < m_FileTree.items.size(); i++, items_drawn++)
    {
        D2D1_RECT_F rect = {
            .left = 0.0f,
            .top = (float)(items_drawn * item_height),
            .right = 500.0f,
            .bottom = (float)((items_drawn + 1) * (item_height)),
        };
        const auto& item = m_FileTree.items[i];

        Rect linear_rect = {
            .x = rect.left,
            .y = rect.top,
            .width = rect.right - rect.left,
            .height = rect.bottom - rect.top,
        };

        auto* brush = &m_HoverBrush;

        if (m_MousePosition.has_value() && linear_rect.contains(*m_MousePosition))
        {
            brush = &m_ActiveBrush;
        }

        m_RenderTarget->FillRectangle(rect, *brush);

        if (item.flags & FileTreeItem::Directory)
        {
            m_RenderTarget->DrawText(item.name.c_str(), item.name.length(), m_BoldTextFormat, rect, m_TextBrush);

            if (!(item.flags & FileTreeItem::Open))
                while (i + 1 < m_FileTree.items.size() && m_FileTree.items[i + 1].depth > item.depth)
                    i++;
        }
        else
        {
            m_RenderTarget->DrawText(item.name.c_str(), item.name.length(), m_RegularTextFormat, rect, m_TextBrush);
        }
    }

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
            app = reinterpret_cast<Application*>(cs->lpCreateParams);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
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
        break;
    case WM_DISPLAYCHANGE:
        InvalidateRect(hWnd, nullptr, false);
        break;
    case WM_PAINT:
        app->OnRender();
        ValidateRect(hWnd, nullptr);
        break;
    case WM_MOUSEMOVE:
        {
            if (!app->m_MousePosition.has_value())
            {
                TRACKMOUSEEVENT tme = { sizeof tme };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            app->m_MousePosition = Vec2 {
                (float)GET_X_LPARAM(lParam),
                (float)GET_Y_LPARAM(lParam)
            };
            InvalidateRect(hWnd, nullptr, false);
        }
        break;
    case WM_MOUSELEAVE:
        app->m_MousePosition = std::nullopt;
        break;
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

    IDWriteFactory *direct_write_factory;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_ISOLATED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&direct_write_factory)
    );
    if (!SUCCEEDED(hr))
        return hr;

    m_DirectWriteFactory = direct_write_factory;

    IDWriteTextFormat* text_format_ptr;

    tstring font_family_name = _T("Segoe UI");

    hr = m_DirectWriteFactory->CreateTextFormat(
        font_family_name.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        32.0f, _T("en-au"), &text_format_ptr
    );
    if (!SUCCEEDED(hr))
        return hr;
    m_RegularTextFormat = text_format_ptr;

    hr = m_DirectWriteFactory->CreateTextFormat(
        font_family_name.c_str(), nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        32.0f, _T("en-au"), &text_format_ptr
    );
    if (!SUCCEEDED(hr))
        return hr;
    m_BoldTextFormat = text_format_ptr;

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

    hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x404040), &brush);
    if (!SUCCEEDED(hr))
        return hr;
    m_HoverBrush = brush;

    hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(0x808080), &brush);
    if (!SUCCEEDED(hr))
        return hr;
    m_ActiveBrush = brush;

    hr = m_RenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush);
    if (!SUCCEEDED(hr))
        return hr;
    m_TextBrush = brush;

    return hr;
}

void Application::DiscardDeviceResources()
{
}
