//
// Created by Andrew Huang on 1/23/2022.
//

#include "app.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_dx11.h>
#include <d3dcompiler.h>

#include "vertex.h"

static const char SHADER_SOURCE[] = R"HLSL(
struct vs_input_t {
    float3 localPosition: POSITION;
    float4 color: COLOR0;
    float2 uv: TEXCOORD0;
};

struct v2p_t {
    float4 position: SV_Position;
    float4 color: COLOR0;
    float2 uv: TEXCOORD0;
};

v2p_t VertexMain(vs_input_t input) {
    v2p_t v2p;
    v2p.position = float4(input.localPosition, 1);
    v2p.color = input.color;
    v2p.uv = input.uv;
    return v2p;
}

float4 PixelMain(v2p_t input): SV_Target0 {
    return float4(input.color);
}
)HLSL";

static const VertexPCU vertices[]{
        {{-0.5f, -0.5f, 0.0f}, {255, 0, 0, 255}, {0.0f, 0.0f}},
        {{0.0f, 0.5f, 0.0f}, {0, 255, 0, 255}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0, 0, 255, 255}, {0.0f, 0.0f}},
};

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
            "SD2",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            800,
            600,
            SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );

    createDeviceD3D();

    imguiInit(_window, _device.get(), _context.get());

    createPipeline();
    createVertexBuffer();
    uploadVertexBuffer();
}

App::~App() {
    imguiDestroy();

    SDL_DestroyWindow(_window);

    SDL_Quit();
}

void App::createDeviceD3D() {
    HWND hWnd = getWindowHandle(_window);

    DXGI_SWAP_CHAIN_DESC swapChainDesc{0};

    swapChainDesc.BufferDesc.Width = 0;
    swapChainDesc.BufferDesc.Height = 0;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.OutputWindow = hWnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    UINT deviceFlags = 0;
#ifdef ENGINE_DEBUG_RENDER
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

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
            deviceFlags,
            featureLevels.data(),
            featureLevels.size(),
            D3D11_SDK_VERSION,
            &swapChainDesc,
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
    {
        D3D11_TEXTURE2D_DESC backBufferDesc{0};
        backBuffer->GetDesc(&backBufferDesc);
        _width = static_cast<int>(backBufferDesc.Width);
        _height = static_cast<int>(backBufferDesc.Height);
    }
    _device->CreateRenderTargetView(backBuffer.get(), nullptr, _mainRenderTargetView.put());
}

void App::createPipeline() {
    winrt::com_ptr<ID3DBlob> code;
    winrt::com_ptr<ID3DBlob> errorMessages;

    UINT shaderFlags = 0;
#ifdef ENGINE_DEBUG_RENDER
    shaderFlags |= D3DCOMPILE_DEBUG;
    shaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
    shaderFlags |= D3DCOMPILE_WARNINGS_ARE_ERRORS;
#else
    shaderFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    if (D3DCompile(
            SHADER_SOURCE,
            ARRAYSIZE(SHADER_SOURCE),
            nullptr,
            nullptr,
            nullptr,
            "VertexMain",
            "vs_5_0",
            shaderFlags,
            0,
            code.put(),
            errorMessages.put()
    ) != S_OK) {
        SDL_Log("failed to compile vertex shader: %s", errorMessages->GetBufferPointer());
        exit(EXIT_FAILURE);
    }

    if (_device->CreateVertexShader(
            code->GetBufferPointer(),
            code->GetBufferSize(),
            nullptr,
            _vertexShader.put()
    ) != S_OK) {
        SDL_Log("failed to create vertex shader");
        exit(EXIT_FAILURE);
    }

    const std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDesc{
            {
                    "POSITION",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    0,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0
            },
            {
                    "COLOR",
                    0,
                    DXGI_FORMAT_R8G8B8A8_UNORM,
                    0,
                    D3D11_APPEND_ALIGNED_ELEMENT,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0
            },
            {
                    "TEXCOORD",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    D3D11_APPEND_ALIGNED_ELEMENT,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0
            },
    };

    if (_device->CreateInputLayout(
            inputElementDesc.data(),
            inputElementDesc.size(),
            code->GetBufferPointer(),
            code->GetBufferSize(),
            _inputLayout.put()
    ) != S_OK) {
        SDL_Log("failed to create input layout");
        exit(EXIT_FAILURE);
    }

    // release code & error message blob
    code = nullptr;
    errorMessages = nullptr;

    if (D3DCompile(
            SHADER_SOURCE,
            ARRAYSIZE(SHADER_SOURCE),
            nullptr,
            nullptr,
            nullptr,
            "PixelMain",
            "ps_5_0",
            shaderFlags,
            0,
            code.put(),
            errorMessages.put()
    ) != S_OK) {
        SDL_Log("failed to compile pixel shader: %s", errorMessages->GetBufferPointer());
        exit(EXIT_FAILURE);
    }

    if (_device->CreatePixelShader(
            code->GetBufferPointer(),
            code->GetBufferSize(),
            nullptr,
            _pixelShader.put()
    ) != S_OK) {
        SDL_Log("failed to create pixel shader");
        exit(EXIT_FAILURE);
    }

    D3D11_RASTERIZER_DESC rasterizerDesc{};
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_NONE;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.AntialiasedLineEnable = true;
    if (_device->CreateRasterizerState(&rasterizerDesc, _rasterizer.put()) != S_OK) {
        SDL_Log("failed to create rasterizer state");
        exit(EXIT_FAILURE);
    }
}

void App::createVertexBuffer() {
    D3D11_BUFFER_DESC bufferDesc{0};

    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    if (_device->CreateBuffer(&bufferDesc, nullptr, _vertexBuffer.put()) != S_OK) {
        SDL_Log("failed to create vertex buffer");
        exit(EXIT_FAILURE);
    }
}

void App::uploadVertexBuffer() {
    D3D11_MAPPED_SUBRESOURCE mapped;
    _context->Map(_vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, vertices, sizeof(vertices));
    _context->Unmap(_vertexBuffer.get(), 0);
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
        case SDL_KEYDOWN:
            if (event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                _running = false;
            }
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

        D3D11_VIEWPORT viewport{
                0.0f,
                0.0f,
                static_cast<float>(_width),
                static_cast<float>(_height),
                0.0f,
                1.0f
        };
        _context->RSSetViewports(1, &viewport);

        _context->RSSetState(_rasterizer.get());

        ID3D11Buffer *vertexBuffer = _vertexBuffer.get();
        UINT stride = sizeof(VertexPCU);
        UINT offset = 0;
        _context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);

        _context->IASetInputLayout(_inputLayout.get());

        _context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        _context->VSSetShader(_vertexShader.get(), nullptr, 0);

        _context->PSSetShader(_pixelShader.get(), nullptr, 0);

        _context->ClearRenderTargetView(mainRenderTargetView, _clearColor);

        _context->Draw(ARRAYSIZE(vertices), 0);

        imgui();

        _swapChain->Present(0, 0);
    }
}

void App::imgui() {
    imguiNewFrame();

    ImGui::ColorEdit4("clear color", _clearColor);

    imguiPresent();
}
