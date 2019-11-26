#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>
//win32

BITMAPINFO bitmapInfo;
void *bitmapMemory;
int bitmapWidth;
int bitmapHeight;
int bytesPerPixel = 4;

void render(int xOffset, int yOffset)
{
	int width = bitmapWidth;
	int height = bitmapHeight;

	int pitch = width * bytesPerPixel;
	uint8_t *row = (uint8_t *)bitmapMemory;
	for (int y = 0; y < bitmapHeight; y++) {
		uint8_t *pixel = (uint8_t *)row;
		for (int x = 0; x < bitmapWidth; x++) {
			//					  1  2  3  4
			//pixel in  memory = 00 00 00 00
			//					 xx bb gg rr
			//little endian architecture
			//0xXXRRGGBB

			//blue
			*pixel = (uint8_t)x + xOffset;
			pixel++;

			//green
			*pixel = (uint8_t)y + yOffset;
			pixel++;

			//red
			*pixel = 0;
			pixel++;

			//padding
			*pixel = 0;
			pixel++;
		}
		row += pitch;
	}
}

void resizeDeviceIndependantBitmapSection(int width, int height) {

	if (bitmapMemory) 
	{ 
		VirtualFree(
			/*LPVOID lpAddress*/	bitmapMemory,
			/*SIZE_T dwSize*/		0,
			/*DWORD  dwFreeType*/	MEM_RELEASE
		);
	}

	bitmapWidth = width;
	bitmapHeight = height;

	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = bitmapWidth;
	bitmapInfo.bmiHeader.biHeight = -bitmapHeight; //negative value, so reads top down, origin upper left corner
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	int bytesPerPixel = 4;
	int bitmapMemorySize = width * height * bytesPerPixel;
	bitmapMemory = VirtualAlloc(
		/*LPVOID lpAddress*/		0,
		/*SIZE_T dwSize*/			bitmapMemorySize,
		/*DWORD  flAllocationType*/	MEM_COMMIT,
		/*DWORD  flProtect*/		PAGE_READWRITE
	);		
}

void updateWindow(HDC deviceContext, RECT clientRectangle, int x, int y, int width, int height) 
{
	int windowWidth = clientRectangle.right - clientRectangle.left;
	int windowHeight = clientRectangle.bottom - clientRectangle.top;

	StretchDIBits(
	  /*HDC              hdc*/			deviceContext,
	  /*int              xDest*/		0,
	  /*int              yDest*/		0,
	  /*int              DestWidth*/	windowWidth,
	  /*int              DestHeight*/	windowHeight,
	  /*int              xSrc*/			0,
	  /*int              ySrc*/			0,
	  /*int              SrcWidth*/		bitmapWidth,
	  /*int              SrcHeight*/	bitmapHeight,
	  /*const VOID       *lpBits*/		bitmapMemory,
	  /*const BITMAPINFO *lpbmi*/		&bitmapInfo,
	  /*UINT             iUsage*/		DIB_RGB_COLORS,
	  /*DWORD            rop*/			SRCCOPY
	);
}

LRESULT CALLBACK HandleWindowMessage(_In_ HWND window,_In_ UINT message, _In_ WPARAM wParam,_In_ LPARAM lParam) 
{
	LRESULT Result = 0;
	switch (message)
	{
		case WM_SIZE:
		{			
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;

			RECT clientRect;
			GetClientRect(window, &clientRect);
			updateWindow(deviceContext, clientRect, x, y, width, height);
			EndPaint(window, &paint);
		} break;
		case WM_DESTROY: 
		{
			OutputDebugStringA("WM_DESTROY");
		} break;
		case WM_CLOSE:
		{
			//DestroyWindow(window);
			Result = DefWindowProc(window, message, wParam, lParam);
		} break;
		case WM_ACTIVATEAPP: 
		{
			OutputDebugStringA("WM_ACTIVATEAPP");
		} break;
		default: 
		{
			Result = DefWindowProc(window, message, wParam, lParam);
		} break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nShowCmd) {
	WNDCLASS wc = { };
	wc.lpfnWndProc = HandleWindowMessage;
	wc.hInstance = hInstance;
	wc.lpszClassName = "Sample Window Class";

	RegisterClass(&wc);


	HWND window = CreateWindowEx(
		/*DWORD     dwExStyle*/ 	0,
		/*LPCSTR    lpClassName*/ 	"Sample Window Class",
		/*LPCSTR    lpWindowName*/ 	"Learn to Program Windows",
		/*DWORD     dwStyle*/ 		WS_OVERLAPPEDWINDOW,
		/*int       X*/ 			CW_USEDEFAULT,
		/*int       Y*/ 			CW_USEDEFAULT,
		/*int       nWidth*/ 		CW_USEDEFAULT,
		/*int       nHeight*/ 		CW_USEDEFAULT,
		/*HWND      hWndParent*/ 	NULL,
		/*HMENU     hMenu*/ 		NULL,
		/*HINSTANCE hInstance*/ 	hInstance,
		/*LPVOID    lpParam*/ 		NULL
	);

	if (window == NULL){ return 0; }

	RECT clientRect;
	GetClientRect(window, &clientRect);
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;
	resizeDeviceIndependantBitmapSection(1200, 720);

	ShowWindow(window, nShowCmd);



	MSG message = { };
	int xOffset = 0;
	int yOffset = 0;
	while (1)
	{
		if (message.message == WM_QUIT) { break; }
		if (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) { 
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		render(xOffset, yOffset);
		HDC deviceContext = GetDC(window);

		RECT clientRectangle;
		GetClientRect(window, &clientRectangle);
		int windowWidth = clientRectangle.right - clientRectangle.left;
		int windowHeight = clientRectangle.bottom - clientRectangle.top;
		updateWindow(deviceContext, clientRectangle, 0, 0, windowWidth, windowHeight);
		ReleaseDC(window, deviceContext);
		
		xOffset++;
	}
	return 0;
}