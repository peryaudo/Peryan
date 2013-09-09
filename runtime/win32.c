/* Peryan Runtime Library for x86 Windows */

/* #define DBG_PRINT(TYPE, FUNC_NAME) printf("%s%s\n", #TYPE, #FUNC_NAME) */
#define DBG_PRINT(TYPE, FUNC_NAME)


#include <assert.h>
#include <windows.h>

#include "common.h"

/* for vsnprintf (want to remove in the future) */
#include <stdio.h>

void *PRMalloc(unsigned int size)
{
	HANDLE hHeap = NULL;
	LPVOID res = NULL;

	DBG_PRINT(+, PRMalloc);

	hHeap = GetProcessHeap();
	assert(hHeap != NULL);

	res = HeapAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, size);

	DBG_PRINT(-, PRMalloc);
	return res;
}

void PRFree(void *ptr)
{
	HANDLE hHeap = NULL;

	DBG_PRINT(+, PRMalloc);

	hHeap = GetProcessHeap();
	assert(hHeap != NULL);

	HeapFree(hHeap, 0, ptr);

	DBG_PRINT(-, PRMalloc);
	return;
}

void *PRRealloc(void *ptr, int size)
{
	HANDLE hHeap = NULL;
	LPVOID res = NULL;

	DBG_PRINT(+, PRMalloc);

	hHeap = GetProcessHeap();
	assert(hHeap != NULL);

	res = HeapReAlloc(hHeap, HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, ptr, size);

	DBG_PRINT(-, PRMalloc);
	return res;

}

#define PERYAN_VERSION "Peryan 0.9"

int ginfo_winx = 640;
int ginfo_winy = 480;
int ginfo_cx = 0;
int ginfo_cy = 0;
int ginfo_mesx = 0;
int ginfo_mesy = 0;
int ginfo_r = 0;
int ginfo_g = 0;
int ginfo_b = 0;
int mousex = 0, mousey = 0, mousew = 0;

struct WinPeryanContext {
	HWND hWnd;

	HDC hBufferDC;
	HBITMAP hBufferBitmap;
	HBITMAP hPrevBufferBitmap;

	HDC hRedrawDC;
	HBITMAP hRedrawBitmap;
	HBITMAP hPrevRedrawBitmap;

	int redraw;
};

static struct WinPeryanContext *ctx = NULL;

void InitializeWinPeryanContext()
{
	ctx = PRMalloc(sizeof(struct WinPeryanContext *));
	ctx->hWnd = NULL;

	ctx->hBufferDC = NULL;
	ctx->hBufferBitmap = NULL;
	ctx->hPrevBufferBitmap = NULL;

	ctx->hRedrawDC = NULL;
	ctx->hRedrawBitmap = NULL;
	ctx->hPrevRedrawBitmap = NULL;

	ctx->redraw = 1;

	return;
}

void FinalizeWinPeryanContext()
{
	if (ctx->hBufferDC != NULL) {
		assert(ctx->hBufferBitmap);
		assert(ctx->hPrevBufferBitmap);

		SelectObject(ctx->hBufferDC, ctx->hPrevBufferBitmap);
		DeleteObject(ctx->hBufferBitmap);
		DeleteDC(ctx->hBufferDC);

		ctx->hBufferDC = NULL;
		ctx->hBufferBitmap = NULL;
		ctx->hPrevBufferBitmap = NULL;
	}

	if (ctx->hRedrawDC != NULL) {
		assert(ctx->hRedrawBitmap);
		assert(ctx->hPrevRedrawBitmap);

		SelectObject(ctx->hRedrawDC, ctx->hPrevRedrawBitmap);
		DeleteObject(ctx->hRedrawBitmap);
		DeleteDC(ctx->hRedrawDC);

		ctx->hRedrawDC = NULL;
		ctx->hRedrawBitmap = NULL;
		ctx->hPrevRedrawBitmap = NULL;
	}

	free(ctx);
	ctx = NULL;
	return;
}

void InitializeWindowBuffer()
{
	BITMAPINFO bmi;
	BITMAPINFOHEADER bmih;
	VOID *pvBits = NULL;
	HDC hMainDC = NULL;
	RECT rect;
	ZeroMemory(&bmi, sizeof(bmi));
	ZeroMemory(&bmih, sizeof(bmih));
	ZeroMemory(&rect, sizeof(rect));


	hMainDC = GetDC(ctx->hWnd);
	if (hMainDC == NULL)
		AbortWithErrorMessage("runtime error: cannot obtain device context");

	ctx->hBufferDC = CreateCompatibleDC(hMainDC);
	ctx->hRedrawDC = CreateCompatibleDC(hMainDC);

	ReleaseDC(ctx->hWnd, hMainDC);

	bmih.biSize = sizeof(bmih);
	bmih.biWidth = ginfo_winx;
	bmih.biHeight = ginfo_winy;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;

	bmi.bmiHeader = bmih;

	ctx->hBufferBitmap = CreateDIBSection(ctx->hBufferDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	ctx->hRedrawBitmap = CreateDIBSection(ctx->hRedrawDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (ctx->hBufferBitmap == NULL || ctx->hRedrawBitmap == NULL)
		AbortWithErrorMessage("runtime error: cannot create DIB section");

	ctx->hPrevBufferBitmap = SelectObject(ctx->hBufferDC, ctx->hBufferBitmap);
	ctx->hPrevRedrawBitmap = SelectObject(ctx->hRedrawDC, ctx->hRedrawBitmap);

	rect.left = rect.top = 0;
	rect.right = ginfo_winx;
	rect.bottom = ginfo_winy;
	FillRect(ctx->hBufferDC, &rect, GetStockObject(WHITE_BRUSH));
	FillRect(ctx->hRedrawDC, &rect, GetStockObject(WHITE_BRUSH));

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


void RefreshMainDC() {
	RECT rect;
	rect.top = rect.left = 0;
	rect.right = ginfo_winx;
	rect.bottom = ginfo_winy;
	InvalidateRect(ctx->hWnd, &rect, FALSE);

	ProcessWindowMessages();
	return;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HDC hMainDC = NULL;
	PAINTSTRUCT ps;

	switch (uMsg) {
	case WM_CREATE:
		InitializeWindowBuffer();
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		FinalizeWinPeryanContext();
		break;

	case WM_PAINT:
		hMainDC = BeginPaint(ctx->hWnd, &ps);
		BitBlt(hMainDC, 0, 0, ginfo_winx, ginfo_winy, ctx->hBufferDC, 0, 0, SRCCOPY);
		EndPaint(ctx->hWnd, &ps);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

void AbortWithErrorMessage(const char *format, ...)
{
	va_list arg;
	char msg[2048];

	va_start(arg, format);
	vsnprintf(msg, sizeof(msg) / sizeof(msg[0]), format, arg);
	va_end(arg);

	MessageBox(NULL, msg, PERYAN_VERSION, MB_OK | MB_ICONWARNING);
	ExitProcess(1);

	return;
}

void InitializeWindow(HINSTANCE hInstance)
{
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "WinPeryanMainWindowClass";
	
	if (RegisterClass(&wc) == 0) {
		AbortWithErrorMessage("runtime error: cannot register the window class");
	}

	ctx->hWnd = CreateWindow(
			wc.lpszClassName,
			PERYAN_VERSION,
			WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			ginfo_winx,
			ginfo_winy,
			NULL,
			NULL,
			hInstance,
			NULL);

	if (ctx->hWnd == NULL) {
		AbortWithErrorMessage("runtime error: cannot create a window");
	}

	ShowWindow(ctx->hWnd, SW_SHOW);
	UpdateWindow(ctx->hWnd);

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

	assert(!"never reaches there");

	DBG_PRINT(-, WinMain);
	return 0;
}

HDC GetCurrentDC() {
	if (ctx->redraw == 1) {
		return ctx->hBufferDC;
	} else {
		return ctx->hRedrawDC;
	}
}

void mes(struct String *str)
{
	LONG res = 0;
	DBG_PRINT(+, mes);

	SetTextColor(GetCurrentDC(), RGB(ginfo_r, ginfo_g, ginfo_b));
	res = TabbedTextOut(GetCurrentDC(), ginfo_cx, ginfo_cy, str->str, str->length, 0, NULL, 0);
	
	ginfo_mesx = res & 0xFFFF;
	ginfo_mesy = (res & 0xFFFF0000) >> 16;

	ginfo_cy += ginfo_mesy;

	RECT rect;
	rect.top = 0; rect.left = 0; rect.right = ginfo_winx; rect.bottom = ginfo_winy;

	RefreshMainDC();

	DBG_PRINT(-, mes);
	return;
}

void pos(int x, int y)
{
	ginfo_cx = x;
	ginfo_cy = y;
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

void redraw(int mode) {
	if (mode != 0 && mode != 1) {
		AbortWithErrorMessage(
		"runtime error: use of the number which is "
		"neither 0 nor 1 as a first argument of redraw");
	}

	if (ctx->redraw == 0 && mode == 0) {
		/* nothing to do */

	} else if (ctx->redraw == 0 && mode == 1) {
		/* copy redraw dc to buffer dc*/
		BitBlt(ctx->hBufferDC, 0, 0, ginfo_winx, ginfo_winy, ctx->hRedrawDC, 0, 0, SRCCOPY);

		/* copy buffer dc to main dc */
		RefreshMainDC();

	} else if (ctx->redraw == 1 && mode == 0) {
		/* copy buffer dc to redraw dc */
		BitBlt(ctx->hRedrawDC, 0, 0, ginfo_winx, ginfo_winy, ctx->hBufferDC, 0, 0, SRCCOPY);

	} else if (ctx->redraw == 1 && mode == 1) {
		/* copy buffer dc to main dc */
		RefreshMainDC();
	}

	ctx->redraw = mode;

	return;
}

void color(int r, int g, int b)
{
	if (r != -1) ginfo_r = r;
	if (g != -1) ginfo_g = g;
	if (b != -1) ginfo_b = b;
	return;
}

void boxf(int left, int top, int right, int bottom)
{
	RECT rect;
	HBRUSH hbr = NULL;

	rect.left = left;
	rect.top = top;
	rect.right = right;
	rect.bottom = bottom;

	hbr = CreateSolidBrush(RGB(ginfo_r, ginfo_g, ginfo_b));
	FillRect(GetCurrentDC(), &rect, hbr);
	DeleteObject(hbr);

	RefreshMainDC();

	return;
}

void title(struct String *text)
{
	SetWindowText(ctx->hWnd, text->str);

	return;
}
