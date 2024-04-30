#include "RenderContext.h"

#include <EngineCommon/Config.h>

#include <SDL3/SDL.h>

RenderContext::RenderContext() {}

void RenderContext::InitWindow()
{
    // We initialize SDL and create a window with it.
    SDL_Init(SDL_INIT_VIDEO);

    m_SDLWindow = SDL_CreateWindow("Magic Red", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_VULKAN);
    SDL_SetRelativeMouseMode(SDL_TRUE);

#if defined(SDL_PLATFORM_WIN32)
    hwnd = (HWND)SDL_GetProperty(SDL_GetWindowProperties(m_SDLWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
    m_DiligentHWND = Diligent::Win32NativeWindow(hwnd);
#elif defined(SDL_PLATFORM_MACOS)
    NSWindow *nswindow = (NSWindow *)GetNSWindowView(m_SDLWindow);
    m_DiligentNSWindow = Diligent::MacOSNativeWindow(nswindow);
#endif
}

void RenderContext::DestroyWindow()
{
    SDL_DestroyWindow(m_SDLWindow);
}

void RenderContext::Init()
{
    Diligent::SwapChainDesc swapchainDesc = Diligent::SwapChainDesc(
        WINDOW_WIDTH, WINDOW_HEIGHT, 
        Diligent::TEXTURE_FORMAT::TEX_FORMAT_BGRA8_UNORM, Diligent::TEXTURE_FORMAT::TEX_FORMAT_D32_FLOAT,
        3, //swapChainImageCount,
        1.f, // Default depth value
        0,   // Default stencil value
        true // Is primary
    );
    Diligent::EngineVkCreateInfo engineCI;
    auto* pFactoryVk = Diligent::GetEngineFactoryVk();
    pFactoryVk->CreateDeviceAndContextsVk(engineCI, &m_pDevice, &m_pImmediateContext);

    InitWindow();
#if PLATFORM_WINDOWS
    pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, swapchainDesc, m_DiligentHWND, &m_pSwapChain);
#elif PLATFORM_MACOS
    pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, swapchainDesc, m_DiligentNSWindow, &m_pSwapChain);
#endif
}

Diligent::RefCntAutoPtr<Diligent::IRenderDevice> RenderContext::Device() const
{
    return m_pDevice;
}

Diligent::RefCntAutoPtr<Diligent::IDeviceContext> RenderContext::Context() const
{
    return m_pImmediateContext;
}

Diligent::RefCntAutoPtr<Diligent::ISwapChain> RenderContext::SwapChain() const
{
    return m_pSwapChain;
}
