//
// Created by Andrew Huang on 1/23/2022.
//

#ifndef SD2_APP_H
#define SD2_APP_H

#include <SDL_video.h>
#include <SDL_events.h>
#include <winrt/base.h>
#include <d3d11.h>

class App {
public:
    App();

    App(const App &) = delete;

    App &operator=(const App &) = delete;

    ~App();

    void mainLoop();

private:
    void createDeviceD3D();

    void createRenderTarget();

    void createPipeline();

    void createVertexBuffer();

    void uploadVertexBuffer();

    void handleEvents();

    void handleEvent(SDL_Event &event);

    void handleWindowEvent(SDL_WindowEvent &windowEvent);

    void imgui();

    SDL_Window *_window = nullptr;
    bool _running = false;

    winrt::com_ptr<ID3D11Device> _device;
    winrt::com_ptr<ID3D11DeviceContext> _context;
    winrt::com_ptr<IDXGISwapChain> _swapChain;

    int _width = 0, _height = 0;
    winrt::com_ptr<ID3D11RenderTargetView> _mainRenderTargetView;

    float _clearColor[4]{0.5f, 0.5f, 0.5f, 1.0f};

    winrt::com_ptr<ID3D11VertexShader> _vertexShader;
    winrt::com_ptr<ID3D11InputLayout> _inputLayout;
    winrt::com_ptr<ID3D11PixelShader> _pixelShader;
    winrt::com_ptr<ID3D11RasterizerState> _rasterizer;

    winrt::com_ptr<ID3D11Buffer> _vertexBuffer;
};

#endif //SD2_APP_H
