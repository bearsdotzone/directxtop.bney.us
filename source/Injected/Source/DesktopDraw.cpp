#include "DesktopDraw.h"
#include <tchar.h>
#include <d3d9.h>
#include <Uxtheme.h>
#include <Wininet.h>
#include <Shlobj.h>
#include <iostream>
#include <fstream>
#include <d3dx9.h>
#include <d3dx9tex.h>
#include <math.h>

#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

typedef LRESULT (CALLBACK *WINDOWPROC)(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ListViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DefViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WINDOWPROC originalListViewWindowProc;
WINDOWPROC originalDefViewWindowProc;

HWND listView;
HDC memoryDC;
RECT listViewRect;
IDirect3D9 * d3d;
IDirect3DDevice9 * d3ddev;
IDirect3DTexture9 * tex = NULL;
IDirect3DTexture9 * img = NULL;
void * bits;
D3DCOLOR colors[] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xff000000, 0xff330066, 0xffff33ff, 0xffff8000 };
D3DCOLOR bgcolors[] = { 0xff990000, 0xff336600, 0xff003366, 0xff660066, 0xffa0a0a0, 0xffcce5ff, 0xffffff66, 0xff003333 };
D3DCOLOR a = 0xff0000ff, b = 0xff00ff00, c = 0xffff0000, d1 = 0xffffffff, d2 = 0xffffffff, d3 = 0xffffffff, d4 = 0xffffffff;
float ballX = 0;
float ballY = 0;
float leftPaddleY = 0;
float rightPaddleY = 0;
float dir = 0.05f;
UINT num = 0;
float randomDir = (float)(rand() % 7) / 100.0f;
bool doflash = true;
struct BgVertex {
	float position[3];
	D3DCOLOR colour;
};

struct FgVertex {
	float position[3];
	float uv[2];
};

struct NewTri {
	float position[3];
	D3DCOLOR color;
};


DWORD WINAPI DesktopDraw(LPVOID param)
{
	HMODULE hModule = (HMODULE)param;
	char libName[MAX_PATH]; 
	GetModuleFileNameA(hModule, libName, MAX_PATH);

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

	listView = FindWindowEx(defView, 0, _T("SysListView32"), _T("FolderView"));
	GetClientRect(listView, &listViewRect);

	memoryDC = CreateCompatibleDC(NULL);
	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = listViewRect.right - listViewRect.left;
	bi.bmiHeader.biHeight = listViewRect.bottom - listViewRect.top;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32;
	bi.bmiHeader.biCompression = BI_RGB;
	HBITMAP bitmap = CreateDIBSection(memoryDC, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	SelectObject(memoryDC, bitmap);

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	D3DPRESENT_PARAMETERS presentParams = {
		listViewRect.right - listViewRect.left,
		listViewRect.bottom - listViewRect.top,
		D3DFMT_UNKNOWN,
		1,
		D3DMULTISAMPLE_NONE,
		0,
		D3DSWAPEFFECT_DISCARD,
		listView,
		TRUE,
		FALSE,
		D3DFMT_D24S8,
		0,
		0,
		D3DPRESENT_INTERVAL_DEFAULT
	};
	d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, listView, D3DCREATE_PUREDEVICE | D3DCREATE_HARDWARE_VERTEXPROCESSING, &presentParams, &d3ddev);
	d3ddev->CreateTexture(listViewRect.right - listViewRect.left, listViewRect.bottom - listViewRect.top, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &tex, NULL);

	originalListViewWindowProc = (WINDOWPROC)GetWindowLongPtr(listView, GWLP_WNDPROC);
	SetWindowLongPtr(listView, GWLP_WNDPROC, (LONG_PTR)ListViewWindowProc);

	originalDefViewWindowProc = (WINDOWPROC)GetWindowLongPtr(defView, GWLP_WNDPROC);
	SetWindowLongPtr(defView, GWLP_WNDPROC, (LONG_PTR)DefViewWindowProc);

	InvalidateRect(listView, NULL, TRUE);
	Sleep(10000000000);

	SetWindowLongPtr(defView, GWLP_WNDPROC, (LONG_PTR)originalDefViewWindowProc);
	SetWindowLongPtr(listView, GWLP_WNDPROC, (LONG_PTR)originalListViewWindowProc);
	tex->Release();
	d3ddev->Release();
	d3d->Release();
	DeleteObject(bitmap);
	DeleteObject(memoryDC);

	InvalidateRect(listView, NULL, TRUE);
	
	return 0;
}

LRESULT CALLBACK DefViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if(uMsg == WM_ERASEBKGND) {
		HBRUSH b = CreateSolidBrush(0xff000000);
		FillRect((HDC)wParam, &listViewRect, b);
		DeleteObject(b);
		return 0;
	} else if(uMsg == WM_PAINT) {
		ValidateRgn(hwnd, NULL);
		return 0;
	} else if(uMsg == WM_PRINT || uMsg == WM_PRINTCLIENT) {
		return 0;
	} else {
		return originalDefViewWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
void Tick(bool flash){
	if (flash){
		a = colors[rand() % (sizeof(colors) / sizeof(colors[0]))];
		b = colors[rand() % (sizeof(colors) / sizeof(colors[0]))];
		c = colors[rand() % (sizeof(colors) / sizeof(colors[0]))];
		d1 = bgcolors[rand() % (sizeof(bgcolors) / sizeof(bgcolors[0]))];
		d2 = bgcolors[rand() % (sizeof(bgcolors) / sizeof(bgcolors[0]))];
		d3 = bgcolors[rand() % (sizeof(bgcolors) / sizeof(bgcolors[0]))];
		d4 = bgcolors[rand() % (sizeof(bgcolors) / sizeof(bgcolors[0]))];
	}
}
LRESULT CALLBACK ListViewWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_PAINT) {
		LRESULT rv = originalListViewWindowProc(hwnd, uMsg, (WPARAM)memoryDC, lParam);
		GdiFlush();

		D3DLOCKED_RECT lockedRect;
		//Makes the icons appear. What dark sorcery this is, I know not.
		tex->LockRect(0, &lockedRect, NULL, D3DLOCK_DISCARD);
		unsigned int pitch = (listViewRect.right - listViewRect.left) * 4;
		for (unsigned int y = 0; y < listViewRect.bottom - listViewRect.top; ++y) {
			memcpy(((unsigned char *)lockedRect.pBits) + lockedRect.Pitch * y, ((unsigned char *)bits) + pitch * (listViewRect.bottom - listViewRect.top - y - 1), pitch);
		}
		tex->UnlockRect(0);
		D3DXCreateTextureFromFile(d3ddev, L"C:/Users/Karbon/Desktop/pong/_0126_Layer 1.bmp", &img);
		const BgVertex bgv[4] = {
				{ { -1.0f, 1.0f, 0.0f }, 0xffff0000 },
				{ { 1.0f, 1.0f, 0.0f }, 0xff00ff00 },
				{ { -1.0f, -1.0f, 0.0f }, 0xff0000ff },
				{ { 1.0f, -1.0f, 0.0f }, 0xffffffff }
		};

		const FgVertex fgv[4] = {
				{ { -1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f } },
				{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
				{ { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },
				{ { 1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } }
		};

		const NewTri meh[4] = {
				{ { -1.0f, 1.0f, 0.0f }, d1 },
				{ { 1.0f, 1.0f, 0.0f }, d2},
				{ { -1.0f, -1.0f, 0.0f }, d3},
				{ { 1.0f, -1.0f, 0.0f }, d4}
		};

		NewTri leftPaddle[4] = {
				{ { -0.9f, 0.4f + leftPaddleY, 0.0f }, a },
				{ { -0.85f, 0.4f + leftPaddleY, 0.0f }, a },
				{ { -0.9f, -0.4f + leftPaddleY, 0.0f }, a },
				{ { -0.85f, -0.4f + leftPaddleY, 0.0f }, a }
		};

		NewTri rightPaddle[4] = {
				{ { 0.85f, 0.4f + rightPaddleY, 0.0f }, b },
				{ { 0.9f, 0.4f + rightPaddleY, 0.0f }, b },
				{ { 0.85f, -0.4f + rightPaddleY, 0.0f }, b },
				{ { 0.9f, -0.4f + rightPaddleY, 0.0f }, b }
		};

		NewTri ball[4] = {
				{ { ((listViewRect.bottom - (float)listViewRect.top) / (listViewRect.right - listViewRect.left)) * -0.05f + ballX, 0.05f + ballY, 0.0f }, c },
				{ { ((listViewRect.bottom - (float)listViewRect.top) / (listViewRect.right - listViewRect.left)) * 0.05f + ballX, 0.05f + ballY, 0.0f }, c },
				{ { ((listViewRect.bottom - (float)listViewRect.top) / (listViewRect.right - listViewRect.left)) * -0.05f + ballX, -0.05f + ballY, 0.0f }, c },
				{ { ((listViewRect.bottom - (float)listViewRect.top) / (listViewRect.right - listViewRect.left)) * 0.05f + ballX, -0.05f + ballY, 0.0f }, c }
		};
		d3ddev->BeginScene();
		d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, 0xff00ff00, 0.0f, 0);
		d3ddev->SetRenderState(D3DRS_LIGHTING, FALSE);
		d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		d3ddev->SetTexture(0, NULL);
		d3ddev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, meh, sizeof(NewTri));
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, leftPaddle, sizeof(NewTri));
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, rightPaddle, sizeof(NewTri));
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, ball, sizeof(NewTri));
		d3ddev->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1);
		//d3ddev->SetTexture(0, img);

		d3ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		d3ddev->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		d3ddev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		d3ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		d3ddev->SetTexture(0, tex);
		d3ddev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, fgv, sizeof(FgVertex));

		d3ddev->EndScene();
		d3ddev->Present(NULL, NULL, NULL, NULL);
		ValidateRgn(hwnd, NULL);
		SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, 0);
		ballX += dir;
		ballY += randomDir;
		if (ballX < -0.85f) {
			dir *= -1;
			Tick(doflash);
		}
		if (ballX > 0.85f) {
			dir *= -1;
			Tick(doflash);
		}
		if (ballY < -1.0f) {
			randomDir = (float)(rand() % 7) / 100.0f;
			Tick(doflash);
		}
		if (ballY > 1.0f) {
			randomDir = (float)(rand() % 7) / 100.0f * -1;
			Tick(doflash);
		}

		//leftPaddleY += (ballY > leftPaddleY) ? 10/sqrt(pow(.125 - (ballX + 1), 2) + pow((leftPaddleY + 1) - (ballY + 1), 2)) : -10/sqrt(pow(.125 - (ballX + 1), 2) + pow((leftPaddleY + 1) - (ballY + 1), 2));
		//rightPaddleY += (ballY > rightPaddleY) ? 10 / sqrt(pow(.125 - (ballX + 1), 2) + pow((rightPaddleY + 1) - (ballY + 1), 2)) : -10 / sqrt(pow(.125 - (ballX + 1), 2) + pow((rightPaddleY + 1) - (ballY + 1), 2));
		if (ballY > leftPaddleY) {
			leftPaddleY -= (0.05f * ((ballX + 1) - 1.875));
		}
		if (ballY < leftPaddleY) {
			leftPaddleY += (0.05f * ((ballX + 1) - 1.875));
		}
				if (leftPaddleY > 0.6f) {
					leftPaddleY = 0.6f;
				}
				if (leftPaddleY < -0.6f) {
					leftPaddleY = -0.6f;
				}
			if (ballY > rightPaddleY) {
				rightPaddleY -= (0.05f * ((ballX + 1) - 2.875));
			}
			if (ballY < rightPaddleY) {
				rightPaddleY += (0.05f * ((ballX + 1) - 2.875));
			}
			if (rightPaddleY > 0.6f) {
				rightPaddleY = 0.6f;
			}
			if (rightPaddleY < -0.6f) {
				rightPaddleY = -0.6f;
			}
			return 0;
		}
		else {
			return originalListViewWindowProc(hwnd, uMsg, wParam, lParam);
		}
	
}
