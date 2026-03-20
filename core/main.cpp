#include <iostream>
#include <Windows.h>
#include <core/protection/anti_patch.h>
#include <core/protection/detection.h>
#include <core/auth/auth.h>
#include <core/crypt/skCrypter.h>
#include <core/utils/utils.h>
#include <core/protection/anti_debug.h>
#include <core/mapper/pe_parse.h>
#include <core/render/render.h>
#include <core/ui/ui.h>
#include <core/mapper/manual_map.h>
#include <fstream>
#include <thread>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	utils::initialize_console();

	anti_patch::initialize();

	DWORD pid = utils::get_process_id_by_name(L"Notepad.exe");

 //   std::thread auth_thread([]() {
 //       auth_success = auth::initialize();
 //       auth_complete = true;

 //       if (!auth_success) {
 //           error_message = "Failed to initialize authentication";
 //       }
 //       });
 //   auth_thread.detach();

	//Render::Initialize();

    std::cin.get();

	return 0;
}