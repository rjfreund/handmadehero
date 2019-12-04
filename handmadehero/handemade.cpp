#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <Xinput.h>
#include <dsound.h>
#pragma comment(lib, "XInput.lib")
#pragma comment(lib, "dsound.lib")
//win32

BITMAPINFO bitmapInfo;
void *bitmapMemory;
int bitmapWidth;
int bitmapHeight;
int bytesPerPixel = 4;

LPDIRECTSOUNDBUFFER secondaryAudioBuffer;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void initializeDirectSound(HWND window, int32_t samplesPerSecond, int32_t bufferSize) {
	HMODULE directSoundLibrary = LoadLibrary("dsound.dll");
	if (!directSoundLibrary) { return; /*error*/ }
	direct_sound_create *directSoundCreate = (direct_sound_create *)GetProcAddress(directSoundLibrary, "DirectSoundCreate");
	LPDIRECTSOUND directSound;
	if (!SUCCEEDED(DirectSoundCreate(0, &directSound, 0))) { return; /* log error maybe */ }
	if (!SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) { return; /*log error*/ }
	DSBUFFERDESC bufferDescription = {};
	bufferDescription.dwSize = sizeof(bufferDescription);
	bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

	LPDIRECTSOUNDBUFFER primaryBuffer;
	if (!SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) { return; }

	WAVEFORMATEX waveFormat = {};
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;
	waveFormat.nChannels = 2;
	waveFormat.nSamplesPerSec = samplesPerSecond;
	waveFormat.wBitsPerSample = 16;
	waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
	waveFormat.cbSize = 0;
	if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) { return; };

	DSBUFFERDESC secondaryBufferDescription = {};
	secondaryBufferDescription.dwSize = sizeof(secondaryBufferDescription);
	secondaryBufferDescription.dwFlags = 0;
	secondaryBufferDescription.dwBufferBytes = bufferSize;
	secondaryBufferDescription.lpwfxFormat = &waveFormat;

	if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDescription, &secondaryAudioBuffer, 0))) { return; }
}

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

LRESULT CALLBACK HandleWindowMessage(_In_ HWND window, _In_ UINT message, _In_ WPARAM wParam, _In_ LPARAM lParam)
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
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_KEYUP:
	{
		uint32_t virtualKeyCode = wParam;
		bool wasDownPrev = (lParam & (1 << 30)) != 0;
		bool isDown = (lParam & (1 << 31)) == 0;
		if (isDown == wasDownPrev) { break; }
		switch (virtualKeyCode) {
		case 'W':
		case 'A':
		case 'S':
		case 'D':
		case 'Q':
		case 'E':
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_ESCAPE:
		case VK_SPACE:
			TCHAR virtualKeyCodeString[256];
			sprintf_s(virtualKeyCodeString, "%c", virtualKeyCode);
			OutputDebugStringA(virtualKeyCodeString);
			OutputDebugStringA("\n");
			break;
		}
	} break;
	default:
	{
		Result = DefWindowProc(window, message, wParam, lParam);
	} break;
	}

	return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
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

	if (window == NULL) { return 0; }

	RECT clientRect;
	GetClientRect(window, &clientRect);
	int width = clientRect.right - clientRect.left;
	int height = clientRect.bottom - clientRect.top;
	resizeDeviceIndependantBitmapSection(1200, 720);

	ShowWindow(window, nShowCmd);



	MSG message = { };
	int xOffset = 0;
	int yOffset = 0;

	int samplesPerSecond = 48000;
	int toneHz = 256;
	int toneVolume = 500;
	uint32_t runningSampleIndex = 0;
	int squareWavePeriod = samplesPerSecond / toneHz;
	int halfSquareWavePeriod = squareWavePeriod / 2;
	int bytesPerSample = sizeof(int16_t) * 2;
	int secondaryBufferSize = samplesPerSecond * bytesPerSample;


	initializeDirectSound(window, samplesPerSecond, samplesPerSecond * bytesPerSample);
	secondaryAudioBuffer->Play(
		/*DWORD dwReserved1*/	0,
		/*DWORD dwPriority*/	0,
		/*DWORD dwFlags*/		DSBPLAY_LOOPING
	);

	while (1)
	{
		if (message.message == WM_QUIT) { break; }
		if (PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		for (int i = 0; i < XUSER_MAX_COUNT; i++) {
			XINPUT_STATE controllerState;
			if (XInputGetState(i, &controllerState) != ERROR_SUCCESS) {
				//controller not plugged in, maybe log error
				continue;
			}
			XINPUT_GAMEPAD *gamepad = &controllerState.Gamepad;
			bool up = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
			bool down = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
			bool left = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
			bool right = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
			bool start = gamepad->wButtons & XINPUT_GAMEPAD_START;
			bool back = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
			bool leftShoulder = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
			bool rightShoulder = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
			bool aButton = gamepad->wButtons & XINPUT_GAMEPAD_A;
			bool bButton = gamepad->wButtons & XINPUT_GAMEPAD_B;
			bool xButton = gamepad->wButtons & XINPUT_GAMEPAD_X;
			bool yButton = gamepad->wButtons & XINPUT_GAMEPAD_Y;

			int16_t stickX = gamepad->sThumbLX;
			int16_t stickY = gamepad->sThumbLY;

			if (aButton) {}

		}

		render(xOffset, yOffset);

		DWORD playCursor;
		DWORD writeCursor;
		if (!SUCCEEDED(secondaryAudioBuffer->GetCurrentPosition(&playCursor, &writeCursor))) { return 0; }

		DWORD byteToLock = (runningSampleIndex * bytesPerSample) % secondaryBufferSize;
		DWORD bytesToWrite;
		if (byteToLock > playCursor) {
			bytesToWrite = secondaryBufferSize - byteToLock;
			bytesToWrite += playCursor;
		}
		else {
			bytesToWrite = playCursor - byteToLock;
		}

		void * region1;
		DWORD region1Size;
		void * region2;
		DWORD region2Size;

		secondaryAudioBuffer->Lock(
			/*DWORD dwOffset*/			byteToLock,
			/*DWORD dwBytes*/			bytesToWrite,
			/*LPVOID * ppvAudioPtr1*/	&region1,
			/*LPDWORD pdwAudioBytes1*/	&region1Size,
			/*LPVOID * ppvAudioPtr2*/	&region2,
			/*LPDWORD pdwAudioBytes2*/	&region2Size,
			/*DWORD dwFlags*/			0
		);

		int16_t *sampleOut = (int16_t *)region1;
		DWORD region1SampleCount = region1Size / bytesPerSample;

		for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; sampleIndex++) {
			int16_t sampleValue = ((runningSampleIndex / halfSquareWavePeriod) % 2) ? toneVolume : -toneVolume;
			*sampleOut = sampleValue;
			*sampleOut++;
			*sampleOut = sampleValue;
			*sampleOut++;
			runningSampleIndex++;
		}

		sampleOut = (int16_t *)region2;
		DWORD region2SampleCount = region2Size / bytesPerSample;
		for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; sampleIndex++) {
			int16_t sampleValue = ((runningSampleIndex / halfSquareWavePeriod) % 2) ? toneVolume : -toneVolume;
			*sampleOut = sampleValue;
			*sampleOut++;
			*sampleOut = sampleValue;
			*sampleOut++;
			runningSampleIndex++;
		}

		secondaryAudioBuffer->Unlock(
			/*LPVOID pvAudioPtr1*/	region1,
			/*DWORD dwAudioBytes1*/	region1Size,
			/*LPVOID pvAudioPtr2*/	region2,
			/*DWORD dwAudioBytes2*/	region2Size
		);


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