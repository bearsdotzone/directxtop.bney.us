#include "Inject.h"
#include <exception>
#include <windows.h>
#include <tchar.h>

void Inject()
{
	HMODULE lib = LoadLibrary(_T("Injected.dll"));
	if(lib == NULL) {
		throw std::exception("Unable to load library");
	}

	HOOKPROC f = (HOOKPROC)GetProcAddress(lib, "HookProc");
	if(f == NULL) {
		throw std::exception("Unable to get address of proc");
	}

	HWND desktop = GetDesktopWindow();
	HWND explorer = FindWindowEx(desktop, 0, _T("Progman"), _T("Program Manager"));
	HWND defView = FindWindowEx(explorer, 0, _T("SHELLDLL_DefView"), 0);
	unsigned int childAfter = 0;
	while(!defView) {
		HWND worker = FindWindowEx(desktop, (HWND)childAfter, _T("WorkerW"), 0);
		if(worker) {
			defView = FindWindowEx(worker, 0, _T("SHELLDLL_DefView"), 0);
		}

		++childAfter;
	}

	HWND listView = FindWindowEx(defView, 0, _T("SysListView32"), _T("FolderView"));

	DWORD destThread = GetWindowThreadProcessId(defView, NULL);
	UINT msg = RegisterWindowMessage(_T("WM_EXPLORERHOOKDETACH"));

	HHOOK hook = SetWindowsHookEx(WH_GETMESSAGE, f, lib, destThread);
	InvalidateRect(defView, NULL, TRUE);
	Sleep(2000000000000000000);
	UnhookWindowsHookEx(hook);
	FreeLibrary(lib);
}
