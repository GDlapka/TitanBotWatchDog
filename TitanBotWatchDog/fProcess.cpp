#include "fProcess.h"


BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
	DWORD       pid = 0;
	PENUMINFO   pInfo = (PENUMINFO)lParam;
	TCHAR       szTitle[_MAX_PATH + 1];

	// sanity checks
	if (pInfo == NULL)
		// stop the enumeration if invalid parameter is given
		return(FALSE);

	// get the processid for this window
	if (!::GetWindowThreadProcessId(hWnd, &pid))
		// this should never occur :-)
		return(TRUE);

	// compare the process ID with the one given as search parameter
	if (pInfo->PId == pid)
	{
		// look for the visibility first
		if (::IsWindowVisible(hWnd))
		{
			// look for the title next
			if (::GetWindowText(hWnd, szTitle, _MAX_PATH) != 0)
			{
				pInfo->hWnd = hWnd;

				// we have found the right window
				return(FALSE);
			}
			else
				pInfo->hEmptyWnd = hWnd;
		}
		else
		{
			// look for the title next
			if (::GetWindowText(hWnd, szTitle, _MAX_PATH) != 0)
			{
				pInfo->hInvisibleWnd = hWnd;
			}
			else
				pInfo->hEmptyInvisibleWnd = hWnd;
		}
	}

	// continue the enumeration
	return(TRUE);
}

//‘ункци€ возвращает hwnd по PId
HWND GetMainWindow(DWORD PId)
{
	ENUMINFO EnumInfo;

	// set the search parameters
	EnumInfo.PId = PId;

	// set the return parameters to default values
	EnumInfo.hWnd = NULL;
	EnumInfo.hEmptyWnd = NULL;
	EnumInfo.hInvisibleWnd = NULL;
	EnumInfo.hEmptyInvisibleWnd = NULL;

	// do the search among the top level windows
	::EnumWindows((WNDENUMPROC)EnumWindowsProc, (LPARAM)&EnumInfo);

	// return the one found if any
	if (EnumInfo.hWnd != NULL)
		return(EnumInfo.hWnd);
	else if (EnumInfo.hEmptyWnd != NULL)
		return(EnumInfo.hEmptyWnd);
	else if (EnumInfo.hInvisibleWnd != NULL)
		return(EnumInfo.hInvisibleWnd);
	else
		return(EnumInfo.hEmptyInvisibleWnd);
}

//‘ункци€ возвращ€ет PId по имени процесса
DWORD GetMyProcessId(WCHAR* name)
{
	HANDLE         hProcessSnap = NULL;
	PROCESSENTRY32 pe32 = { 0 };

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return 0;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(hProcessSnap, &pe32)) {
		do {
			HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS,
				FALSE, pe32.th32ProcessID);
			CloseHandle(hProcess);
			if (pe32.szExeFile == name) {
				CloseHandle(hProcessSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hProcessSnap, &pe32));
	}
	CloseHandle(hProcessSnap);
	return 0;
}

//‘ункци€ вычисл€ет запущен ли процесс по имени
bool IsProcessRun(const WCHAR * const processName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	bool res;
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &pe);

	while (1) {
		if (wcscmp(pe.szExeFile, processName) == 0) {
			res = true;
			break;
		}
		if (!Process32Next(hSnapshot, &pe)) {
			res = false;
			break;
		}
	}
	CloseHandle(hSnapshot);
	return res;
}

void killProcess(const WCHAR * const processName) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hSnapshot, &pe);

	while (1) {
		if (wcscmp(pe.szExeFile, processName) == 0) {
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,
				FALSE, pe.th32ProcessID);
			TerminateProcess(hProcess, 0);
			CloseHandle(hProcess);
			break;
		}
		if (!Process32Next(hSnapshot, &pe)) {
			break;
		}
	}
	CloseHandle(hSnapshot);
}
