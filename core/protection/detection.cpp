#include "detection.h"
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <thread>
#include <core/crypt/skCrypter.h>


void detection::initialize()
{
	running_games_check();
}

void detection::running_games_check()
{
	while (true) {
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnap == INVALID_HANDLE_VALUE)
		{
			std::wstring msg = _(L"Failed to create handle for process snapshot.").decrypt();
			MessageBox(NULL, msg.c_str(), _(L"Error"), MB_OK | MB_ICONERROR);
			ExitProcess(0);
		}

		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnap, &pe32)) {
			do {

				std::wstring exe_file = pe32.szExeFile;
				if (exe_file == _(L"cs2.exe").decrypt() ||
					exe_file == _(L"FortniteClient-Win64-Shipping.exe").decrypt() ||
					exe_file == _(L"r5apex.exe").decrypt() ||
					exe_file == _(L"RainbowSix.exe").decrypt() ||
					exe_file == _(L"cod.exe").decrypt())
				{
					std::wstring msg = _(L"Please close ").decrypt() + exe_file + _(L" before running the loader").decrypt();
					MessageBox(NULL, msg.c_str(), _(L"Error"), MB_OK | MB_ICONERROR);
					ExitProcess(0);
				}

			} while (Process32Next(hSnap, &pe32));
		}
		CloseHandle(hSnap);
		Sleep(2000);
	}
}