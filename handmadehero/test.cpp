#include <Windows.h>

void foo(void) {
	const char *blah = "hello world\n";
	OutputDebugStringA("hello world\n");
}

int CALLBACK WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	char SmallS; // 8 bits - 256 different values [-128, 127]
	char unsigned SmallU; // 8 bits unsigned - 256 different values [0,255]

	short MediumS; // 16 bits - 65536
	short unsigned MediumU;

	int LargeS; // 32 bits - 4 billion
	int unsigned LargeU;

	char unsigned Test;

	Test = 255;
	Test = Test + 1;
	
	foo();
}