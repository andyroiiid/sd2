//
// Created by Andrew Huang on 1/23/2022.
//

#ifndef PLAYGROUND_APP_H
#define PLAYGROUND_APP_H

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

    void handleEvents();

    void handleEvent(SDL_Event &event);

    void handleWindowEvent(SDL_WindowEvent &windowEvent);

    void imgui();

    SDL_Window *_window = nullptr;
    bool _running = false;

    winrt::com_ptr<ID3D11Device> _device;
    winrt::com_ptr<ID3D11DeviceContext> _context;
    winrt::com_ptr<IDXGISwapChain> _swapChain;

    winrt::com_ptr<ID3D11RenderTargetView> _mainRenderTargetView;

    float _clearColor[4]{0.4f, 0.8f, 1.0f, 1.0f};
};

#endif //PLAYGROUND_APP_H
