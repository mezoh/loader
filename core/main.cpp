#include <Windows.h>
#include <core/auth/auth.h>
#include <core/auth/hwid/hwid.h>
#include <core/crypt/skCrypter.h>
#include <core/utils/utils.h>
#include <core/protection/anti_debug.h>
#include <core/protection/anti_patch.h>
#include <core/render/render.h>
#include <core/ui/ui.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

// Opt-in only when we explicitly want a debug console.
#ifdef DISTORTION_ENABLE_CONSOLE
    utils::initialize_console();
#endif

    anti_debug::initialize();
    anti_patch::initialize();

    // render the login UI — auth happens through UI callbacks
    Render::Initialize();

    return 0;
}
