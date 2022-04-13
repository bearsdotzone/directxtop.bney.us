#include <windows.h>
#include "DesktopDraw.h"
#include <tchar.h>
#include <Psapi.h>

EXTERN_C
{
__declspec(dllexport) LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam);
}

bool AmIExplorer();

UINT unhookMessage;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		if(AmIExplorer()) {
			HANDLE thread = CreateThread(NULL, 0, DesktopDraw, hModule, 0, NULL);
			CloseHandle(thread);
		}
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

__declspec(dllexport) LRESULT CALLBACK HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	return CallNextHookEx(NULL, code, wParam, lParam);
}


bool AmIExplorer()
{
	HANDLE process = GetCurrentProcess();
	HMODULE module;
	DWORD modulesSize;
	if(EnumProcessModules(process, &module, sizeof(module), &modulesSize) && modulesSize > 0)
	{
		TCHAR baseName[1024];
		GetModuleBaseName(process, module, baseName, 1024);
		if(_tcsicmp(_T("explorer.exe"), baseName) == 0) {
			return true;
		}
	}

	return false;
}