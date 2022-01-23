//
// Created by Andrew Huang on 1/23/2022.
//

#include "app.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_dx11.h>

static HWND getWindowHandle(SDL_Window *window) {
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version)
    SDL_GetWindowWMInfo(window, &wmInfo);
    return wmInfo.info.win.window;
}

static void imguiInit(SDL_Window *window, ID3D11Device *device, ID3D11DeviceContext *context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplSDL2_InitForD3D(window);
    ImGui_ImplDX11_Init(device, context);
}

static void imguiDestroy() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
}

static void imguiNewFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

static void imguiPresent() {
    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

App::App() {
    SDL_Init(SDL_INIT_EVERYTHING);

    _window = SDL_CreateWindow(
            "DX11",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    createDeviceD3D();

    imguiInit(_window, _device.get(), _context.get());
}

App::~App() {
    imguiDestroy();

    SDL_DestroyWindow(_window);

    SDL_Quit();
}

void App::createDeviceD3D() {
    HWND hWnd = getWindowHandle(_window);

    DXGI_SWAP_CHAIN_DESC sd{0};

    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2;
    sd.OutputWindow = hWnd;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    const std::vector<D3D_FEATURE_LEVEL> featureLevels{
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
    };

    D3D_FEATURE_LEVEL featureLevel;
    if (D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            0,
            featureLevels.data(),
            featureLevels.size(),
            D3D11_SDK_VERSION,
            &sd,
            _swapChain.put(),
            _device.put(),
            &featureLevel,
            _context.put()
    ) != S_OK) {
        SDL_Log("failed to create D3D11 device and swapChain");
        exit(EXIT_FAILURE);
    }
}

void App::createRenderTarget() {
    // release references
    _context->ClearState();
    _mainRenderTargetView = nullptr;

    // automatically resize
    _swapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

    // recreate render target view
    winrt::com_ptr<ID3D11Texture2D> backBuffer;
    _swapChain->GetBuffer(0, __uuidof(backBuffer), backBuffer.put_void());
    _device->CreateRenderTargetView(backBuffer.get(), nullptr, _mainRenderTargetView.put());
}

void App::handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handleEvent(event);
    }
}

void App::handleEvent(SDL_Event &event) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    switch (event.type) {
        case SDL_QUIT:
            _running = false;
            break;
        case SDL_WINDOWEVENT:
            handleWindowEvent(event.window);
            break;
        default:
            break;
    }
}

void App::handleWindowEvent(SDL_WindowEvent &windowEvent) {
    switch (windowEvent.event) {
        case SDL_WINDOWEVENT_RESIZED:
            createRenderTarget();
            break;
        default:
            break;
    }
}

void App::mainLoop() {
    _running = true;
    while (_running) {
        handleEvents();

        _context->ClearState();

        ID3D11RenderTargetView *mainRenderTargetView = _mainRenderTargetView.get();
        _context->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
        _context->ClearRenderTargetView(mainRenderTargetView, _clearColor);

        imgui();

        _swapChain->Present(1, 0);
    }
}

void App::imgui() {
    imguiNewFrame();

    ImGui::ColorEdit4("clear color", _clearColor);

    imguiPresent();
}
