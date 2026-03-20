#include "anti_debug.h"
#include <Windows.h>

void anti_debug::initialize()
{

}

bool anti_debug::simple_is_debugger_present()
{
	if (IsDebuggerPresent()) {
		return true;
	}
	return false;
}

