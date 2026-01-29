#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <openvr.h>
#include <stdint.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
    const wchar_t kWindowClassName[] = L"OpenVRMirrorViewerD3D11";
}

struct D3DState
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;
    DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct MirrorState
{
    ID3D11Texture2D* texture = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM;
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}

bool CreateD3D(HWND hwnd, uint32_t width, uint32_t height, DXGI_FORMAT format, D3DState* out)
{
    if (!out)
    {
        return false;
    }

    if (out->rtv)
    {
        out->rtv->Release();
        out->rtv = nullptr;
    }
    if (out->swap_chain)
    {
        out->swap_chain->Release();
        out->swap_chain = nullptr;
    }
    if (out->context)
    {
        out->context->Release();
        out->context = nullptr;
    }
    if (out->device)
    {
        out->device->Release();
        out->device = nullptr;
    }

    DXGI_SWAP_CHAIN_DESC swap_desc{};
    swap_desc.BufferCount = 2;
    swap_desc.BufferDesc.Width = width;
    swap_desc.BufferDesc.Height = height;
    swap_desc.BufferDesc.Format = format;
    swap_desc.BufferDesc.RefreshRate.Numerator = 60;
    swap_desc.BufferDesc.RefreshRate.Denominator = 1;
    swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swap_desc.OutputWindow = hwnd;
    swap_desc.SampleDesc.Count = 1;
    swap_desc.Windowed = TRUE;
    swap_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    const UINT device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swap_chain = nullptr;
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        device_flags,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swap_desc,
        &swap_chain,
        &device,
        nullptr,
        &context);

    if (FAILED(hr))
    {
        return false;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
    if (FAILED(hr))
    {
        swap_chain->Release();
        context->Release();
        device->Release();
        return false;
    }

    ID3D11RenderTargetView* rtv = nullptr;
    hr = device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
    back_buffer->Release();
    if (FAILED(hr))
    {
        swap_chain->Release();
        context->Release();
        device->Release();
        return false;
    }

    out->device = device;
    out->context = context;
    out->swap_chain = swap_chain;
    out->rtv = rtv;
    out->format = format;
    out->width = width;
    out->height = height;

    return true;
}

void DestroyD3D(D3DState* state)
{
    if (!state)
    {
        return;
    }
    if (state->rtv)
    {
        state->rtv->Release();
    }
    if (state->swap_chain)
    {
        state->swap_chain->Release();
    }
    if (state->context)
    {
        state->context->Release();
    }
    if (state->device)
    {
        state->device->Release();
    }
    *state = D3DState{};
}

bool TryAcquireMirrorTexture(D3DState* d3d, MirrorState* mirror)
{
    if (!d3d || !mirror)
    {
        return false;
    }

    if (mirror->texture)
    {
        return true;
    }

    if (!vr::VRCompositor())
    {
        return false;
    }

    ID3D11Texture2D* texture = nullptr;
    const vr::EVRCompositorError err = vr::VRCompositor()->GetMirrorTextureD3D11(
        vr::Eye_Left,
        d3d->device,
        reinterpret_cast<void**>(&texture));

    if (err != vr::VRCompositorError_None || !texture)
    {
        return false;
    }

    D3D11_TEXTURE2D_DESC desc{};
    texture->GetDesc(&desc);

    mirror->texture = texture;
    mirror->width = desc.Width;
    mirror->height = desc.Height;
    mirror->format = desc.Format;

    return true;
}

void ReleaseMirrorTexture(MirrorState* mirror)
{
    if (!mirror)
    {
        return;
    }
    if (mirror->texture)
    {
        if (vr::VRCompositor())
        {
            vr::VRCompositor()->ReleaseMirrorTextureD3D11(mirror->texture);
        }
        mirror->texture->Release();
    }
    *mirror = MirrorState{};
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
    vr::EVRInitError vr_error = vr::VRInitError_None;
    vr::VR_Init(&vr_error, vr::VRApplication_Background);
    if (vr_error != vr::VRInitError_None)
    {
        MessageBoxA(nullptr, vr::VR_GetVRInitErrorAsEnglishDescription(vr_error), "VR_Init failed", MB_OK);
        return 1;
    }

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = kWindowClassName;
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"OpenVR Mirror Viewer (D3D11)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd)
    {
        vr::VR_Shutdown();
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);

    D3DState d3d;
    MirrorState mirror;

    if (!CreateD3D(hwnd, 1280, 720, DXGI_FORMAT_B8G8R8A8_UNORM, &d3d))
    {
        MessageBoxA(hwnd, "Failed to create D3D11 device", "Error", MB_OK);
        vr::VR_Shutdown();
        return 1;
    }

    MSG msg{};
    bool running = true;
    while (running)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running)
        {
            break;
        }

        if (TryAcquireMirrorTexture(&d3d, &mirror))
        {
            if (mirror.width != d3d.width || mirror.height != d3d.height || mirror.format != d3d.format)
            {
                CreateD3D(hwnd, mirror.width, mirror.height, mirror.format, &d3d);
            }

            ID3D11Texture2D* back_buffer = nullptr;
            HRESULT hr = d3d.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer));
            if (SUCCEEDED(hr) && back_buffer)
            {
                d3d.context->CopyResource(back_buffer, mirror.texture);
                back_buffer->Release();
            }
        }
        else
        {
            const float clear_color[4] = { 0.f, 0.f, 0.f, 1.f };
            d3d.context->OMSetRenderTargets(1, &d3d.rtv, nullptr);
            d3d.context->ClearRenderTargetView(d3d.rtv, clear_color);
        }

        d3d.swap_chain->Present(1, 0);
        Sleep(1);
    }

    ReleaseMirrorTexture(&mirror);
    DestroyD3D(&d3d);

    vr::VR_Shutdown();
    return 0;
}
