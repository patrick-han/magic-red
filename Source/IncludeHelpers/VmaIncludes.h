#include <Common/Compiler/DisableWarnings.h>
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4127) // conditional expression is constant
DISABLE_MSVC_WARNING(4100) // unreferenced formal parameter
DISABLE_MSVC_WARNING(4189) // local variable is initialized but not referenced
DISABLE_MSVC_WARNING(4324) // structure was padded due to alignment specifier
DISABLE_MSVC_WARNING(4820) // 'X': 'N' bytes padding added after data member 'X'
#include <vk_mem_alloc.h>
POP_MSVC_WARNINGS