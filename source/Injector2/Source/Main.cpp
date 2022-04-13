#include <windows.h>
#include "Inject.h"
#include <exception>
#include <sstream>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	try {
		const wchar_t* out = L"123545678901";
		OutputDebugString(out);
		Inject();
	} catch(std::exception & e) {
		std::ostringstream ss;
		ss << "Unable to inject: " << e.what();
		MessageBoxA(NULL, ss.str().c_str(), "Error", MB_OK | MB_ICONEXCLAMATION);
	}
}