#pragma once
#include <EngineCommon/Platform.h>
#include <EngineCommon/Compiler/DisableWarnings.h>

// Diligent
PUSH_CLANG_WARNINGS
DISABLE_CLANG_WARNING("-Wunused-parameter")
DISABLE_CLANG_WARNING("-Wgnu-zero-variadic-macro-arguments")
#include <EngineFactoryVk.h>
#include <Common/interface/RefCntAutoPtr.hpp>
POP_CLANG_WARNINGS

struct SDL_Window;
#if PLATFORM_WINDOWS
    typedef void* HWND;
#elif PLATFORM_MACOS
    extern void* GetNSWindowView(SDL_Window* wnd); // Written in the Objective C++ file SurfaceHelper.mm
    typedef void* NSWindow;
#endif

class RenderContext
{
public:
    RenderContext();
    void Init();

    RenderContext(RenderContext const&) = delete; // Copy constructor
    void operator=(RenderContext const&) = delete; // Copy assignment

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> Device() const;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> Context() const;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> SwapChain() const;

    // TODO: Make this more elegant
    void DestroyWindow();

private:
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice>  m_pDevice;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> m_pImmediateContext;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain>     m_pSwapChain;


    void InitWindow();
    
    SDL_Window *m_SDLWindow;
#if PLATFORM_WINDOWS
    HWND hwnd;
    Diligent::Win32NativeWindow m_DiligentHWND;
#elif PLATFORM_MACOS
    Diligent::MacOSNativeWindow m_DiligentNSWindow;
#endif
};