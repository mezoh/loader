#include "anti_debug.h"

#include <Windows.h>
#include <core/crypt/lazy_importer.hpp>
#include <core/crypt/skCrypter.h>

// NtQueryInformationProcess is not in the standard SDK headers —
// resolve it manually from ntdll at runtime.
typedef LONG(NTAPI* NtQIP_t)(HANDLE, UINT, PVOID, ULONG, PULONG);

// Main thread handle for debug register checks from background thread
static HANDLE g_main_thread = nullptr;

// Magic number for NtQueryInformationProcess
constexpr UINT kProcessDebugPort = 7;

static NtQIP_t resolve_ntqip()
{
    static HMODULE ntdll = LI_FN(GetModuleHandleW)(L"ntdll.dll");
    static auto fn = reinterpret_cast<NtQIP_t>(
        LI_FN(GetProcAddress)(ntdll, _("NtQueryInformationProcess").decrypt()));
    return fn;
}

static void kill()
{
    LI_FN(ExitProcess)(0);
}

// Check 1: PEB BeingDebugged flag (basic, but fast)
static bool is_debugger_present_basic()
{
    return LI_FN(IsDebuggerPresent)() != FALSE;
}

// Check 2: Remote debugger via a different kernel code path than check 1
static bool is_remote_debugger_present()
{
    BOOL present = FALSE;
    LI_FN(CheckRemoteDebuggerPresent)(GetCurrentProcess(), &present);
    return present != FALSE;
}

// Check 3: Debug port — non-null means a kernel debugger or usermode debugger
// with a debug port is attached. Not patchable via PEB alone.
static bool has_debug_port()
{
    auto fn = resolve_ntqip();
    if (!fn) return false;
    HANDLE port = nullptr;
    LONG status = fn(GetCurrentProcess(), kProcessDebugPort,
                     &port, sizeof(port), nullptr);
    return (status == 0 && port != nullptr);
}

// Check 4: Hardware breakpoints set in debug registers Dr0–Dr3
static bool has_hardware_breakpoints()
{
    CONTEXT ctx = {};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    HANDLE thread = g_main_thread ? g_main_thread : LI_FN(GetCurrentThread)();
    if (!LI_FN(GetThreadContext)(thread, &ctx))
        return false;
    return ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0;
}

// Check 5: NtGlobalFlag in PEB — Windows sets heap debug flags (0x70)
// when a debugger launches the process.
static bool has_nt_global_flag()
{
#ifdef _WIN64
    DWORD flag = *reinterpret_cast<DWORD*>(__readgsqword(0x60) + 0xBC);
#else
    DWORD flag = *reinterpret_cast<DWORD*>(__readfsdword(0x30) + 0x68);
#endif
    return (flag & 0x70) != 0;
}

static void run_all_checks()
{
    if (is_debugger_present_basic())  kill();
    if (is_remote_debugger_present()) kill();
    if (has_debug_port())             kill();
    if (has_hardware_breakpoints())   kill();
    if (has_nt_global_flag())         kill();
}

static DWORD WINAPI debug_thread(LPVOID)
{
    while (true) {
        run_all_checks();
        LI_FN(Sleep)(2000);
    }
    return 0;
}

void anti_debug::initialize()
{
    g_main_thread = LI_FN(OpenThread)(THREAD_GET_CONTEXT, FALSE, LI_FN(GetCurrentThreadId)());
    run_all_checks();
    HANDLE thread = LI_FN(CreateThread)(nullptr, 0, debug_thread, nullptr, 0, nullptr);
    if (thread) LI_FN(CloseHandle)(thread);
}

bool anti_debug::simple_is_debugger_present()
{
    return is_debugger_present_basic();
}
