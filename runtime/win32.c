/* Peryan Runtime Library for x86 Windows */

/* #define DBG_PRINT(TYPE, FUNC_NAME) printf("%s%s\n", #TYPE, #FUNC_NAME) */
#define DBG_PRINT(TYPE, FUNC_NAME)

#include "common.h"

#include <windows.h>

extern void PeryanMain();

struct WinPeryanContext {
	HWND hWnd;
};

struct WinPeryanContext *ctx;

void InitializeWinPeryanContext()
{
	ctx = PRMalloc(sizeof(struct WinPeryanContext *));
	ctx->hWnd = NULL;

	return;
}

void FinalizeWinPeryanContext()
{
	free(ctx);
	ctx = NULL;

	return;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void AbortWithErrorMessage()
{
}

/* TODO: add error diagnose function */

void InitializeWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "WinPeryanMainWindowClass";
	
	RegisterClass(&wc);
	/* TODO: add error checking */

	ctx->hWnd = CreateWindow(
			wc.lpszClassName,
			"Peryan 0.9",
			WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			640,
			480,
			NULL,
			NULL,
			hInstance,
			NULL);
	/* TODO: add error checking */

	ShowWindow(ctx->hWnd, SW_SHOW);
	UpdateWindow(ctx->hWnd);

	return;
}

void ProcessWindowMessages()
{
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)) {

		if (!GetMessage(&msg, NULL, 0, 0))
			ExitProcess(msg.wParam);

		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return;
}

void end()
{
	PostQuitMessage(0);
	ProcessWindowMessages();
}

void stop()
{
	while (1) {
		Sleep(5);
		ProcessWindowMessages();
	}

	return;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DBG_PRINT(+, WinMain);

	InitializeWinPeryanContext();

	InitializeWindow(hInstance);

	PeryanMain();

	stop();

	/* never reaches there */
	FinalizeWinPeryanContext();

	DBG_PRINT(-, WinMain);
	return 0;
}

void mes(struct String *str)
{
	DBG_PRINT(+, mes);
	assert(!"no windows runtime implementation");
	DBG_PRINT(-, mes);
	return;
}

int exec(struct String *str)
{
	DBG_PRINT(+, exec);
	assert(!"no windows runtime implementation");
	DBG_PRINT(-, exec);
	return -1;
}

void dirlist(struct String **res, struct String *mask, int mode)
{
	DBG_PRINT(+, dirlist);
	assert(!"no windows runtime implementation");
	DBG_PRINT(-, dirlist);
	return;
}

