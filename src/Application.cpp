#include "Application.hpp"

#include <windowsx.h>
#include <dwrite.h>
#include <tchar.h>
#include <iostream>

using namespace ed;

Application::Application()
    : hwnd(nullptr)
    , mouse_position(std::nullopt)
    , common_state()
    , file_tree_component(&common_state)
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

    hwnd = CreateWindow(
        _T("Direct2DDemo"),
        _T("Direct2d Demo Application"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, nullptr, nullptr,
        instance,
        this
    );

    SetWindowPos(hwnd, nullptr, 0, 0, 720, 480, SWP_NOMOVE);
    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);

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

    auto& render_target = common_state.render_target;

    render_target->BeginDraw();
    render_target->SetTransform(D2D1::Matrix3x2F::Identity());
    render_target->Clear(D2D1::ColorF(0x202020));

    // mock data
    Size bounds = { 1000, 500 };

    file_tree_component.update(bounds, mouse_position, mouse_down);

    hr = render_target->EndDraw();

    if (hr == D2DERR_RECREATE_TARGET)
    {
        std::cerr <<  "Unhandled: D2DERR_RECREATE_TARGET\n";
        return hr;
    }

    return hr;
}

void Application::OnResize(unsigned int width, unsigned int height)
{
    if (!common_state.render_target.empty())
        common_state.render_target->Resize(D2D1::SizeU(width, height));
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
            if (!app->mouse_position.has_value())
            {
                TRACKMOUSEEVENT tme = { sizeof tme };
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hWnd;
                TrackMouseEvent(&tme);
            }
            app->mouse_position = Vec2 {
                (float)GET_X_LPARAM(lParam),
                (float)GET_Y_LPARAM(lParam)
            };
            InvalidateRect(hWnd, nullptr, false);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            app->mouse_down = true;
            InvalidateRect(hWnd, nullptr, false);
        }
        break;
    case WM_LBUTTONUP:
        {
            app->mouse_down = false;
            InvalidateRect(hWnd, nullptr, false);
        }
        break;
    case WM_MOUSELEAVE:
        app->mouse_position = std::nullopt;
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

    common_state.direct2d_factory = factory;

    IDWriteFactory *direct_write_factory;

    hr = DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_ISOLATED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&direct_write_factory)
    );
    if (!SUCCEEDED(hr))
        return hr;

    common_state.direct_write_factory = direct_write_factory;

    return hr;
}

HRESULT Application::CreateDeviceResources()
{
    HRESULT hr = S_OK;
    ID2D1HwndRenderTarget *renderTarget;
    RECT rect;
    D2D1_SIZE_U size;

    if (!common_state.render_target.empty())
        return S_OK;

    GetClientRect(hwnd, &rect);
    size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

    hr = common_state.direct2d_factory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(),
        D2D1::HwndRenderTargetProperties(hwnd, size),
        &renderTarget
    );
    if (!SUCCEEDED(hr))
        return hr;
    common_state.render_target = renderTarget;

    return hr;
}

void Application::DiscardDeviceResources()
{
}
