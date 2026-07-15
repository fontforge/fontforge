#include <windows.h>
#include <Strsafe.h>

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR lpCmdLine, int nCmdShow) { 
	// Define Variables
	wchar_t wszAppPath[MAX_PATH];
	wchar_t wszCmdPath[MAX_PATH];
	wchar_t wszCmdArgs[8192];
	wchar_t *pwszTail;
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { 0 };
	DWORD dwRet;

	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	//Get the path to cmd.exe
	dwRet = GetSystemDirectory(wszAppPath, MAX_PATH);
	if (dwRet == 0 || dwRet >= MAX_PATH) {
		MessageBox(NULL, L"Path too long - could not determine system directory.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	dwRet = StringCchPrintf(wszCmdPath, MAX_PATH, L"%s\\cmd.exe", wszAppPath);
	if (FAILED(dwRet)) {
		MessageBox(NULL, L"Path too long - could not get path to command line interpreter (cmd.exe).",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
  
	// Get path of executable
	dwRet = GetModuleFileName(NULL, wszAppPath, MAX_PATH);
	if (dwRet == 0 || dwRet >= MAX_PATH) {
		MessageBox(NULL, L"Path too long - place FontForge in another directory.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	} else if ((pwszTail = wcsrchr(wszAppPath, L'\\')) == NULL) {
		MessageBox(NULL, L"Could not determine executable location.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	//Construct the parameters to pass to cmd.exe
	*pwszTail = L'\0';
	dwRet = StringCchPrintf(wszCmdArgs, sizeof(wszCmdArgs)/sizeof(wchar_t), L"/c \"\"%s\\fontforge.bat\" %s\"",
		wszAppPath, lpCmdLine);
	if (FAILED(dwRet)) {
		MessageBox(NULL, L"Command parameter is too long.",
			NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
  
	// Run batch file without visible window
	dwRet = CreateProcess(wszCmdPath, wszCmdArgs, NULL, NULL, FALSE,
		CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
	if (dwRet == 0) {
		MessageBox(NULL, L"Could not launch FontForge.", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	//Wait for the process to end and get the exit code
	dwRet = WaitForSingleObject(pi.hProcess, 10000);
	if (dwRet == WAIT_OBJECT_0) {
		GetExitCodeProcess(pi.hProcess, &dwRet);
		if (dwRet != 0) {
			wchar_t buf[100];
			StringCchPrintf(buf, sizeof(buf)/sizeof(wchar_t), L"Could not launch FontForge (exit code %d).", (int)dwRet);
			MessageBox(NULL, buf, NULL, MB_OK | MB_ICONEXCLAMATION);
		}
	} else {
		dwRet = 0;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return dwRet;
}
