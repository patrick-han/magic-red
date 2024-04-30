#include <SDL3/SDL.h>
#import <Cocoa/Cocoa.h>
#include <EngineCommon/Compiler/DisableWarnings.h>
PUSH_CLANG_WARNINGS
DISABLE_CLANG_WARNING("-Wgnu-statement-expression-from-macro-expansion")

// This code (and comment) are referenced from Diligent Samples, just replace glfw with SDL wordage. Kept here for posterity:
// "
// MoltenVk implement Vulkan API on top of Metal.
// We must add CAMetalLayer to use Metal to render into view.
// Same code implemented into glfwCreateWindowSurface, but DiligentEngine create vulkan surface inside a ISwapChain implementation and can not use the GLFW function.
// "
void* GetNSWindowView(SDL_Window* wnd)
{
    id Window = (NSWindow *)SDL_GetProperty(SDL_GetWindowProperties(wnd), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    id View   = [Window contentView];

    NSBundle* bundle = [NSBundle bundleWithPath:@"/System/Library/Frameworks/QuartzCore.framework"];
    if (!bundle)
        return nullptr;

    id Layer = [[bundle classNamed:@"CAMetalLayer"] layer];
    if (!Layer)
        return nullptr;

    [View setLayer:Layer];
    [View setWantsLayer:YES];

    // Disable retina display scaling
    CGSize viewScale = [View convertSizeToBacking: CGSizeMake(1.0, 1.0)];
    [Layer setContentsScale: MIN(viewScale.width, viewScale.height)];

    return View;
}
POP_CLANG_WARNINGS
