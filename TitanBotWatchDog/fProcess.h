#pragma once
#include <Windows.h>
#include <tlhelp32.h>

// Структура для поиска процесса
typedef struct tagENUMINFO
{
	// In Parameters
	DWORD PId;

	// Out Parameters
	HWND  hWnd;
	HWND  hEmptyWnd;
	HWND  hInvisibleWnd;
	HWND  hEmptyInvisibleWnd;
} ENUMINFO, *PENUMINFO;


BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);

//Функция возвращает hwnd по PId
HWND GetMainWindow(DWORD PId);

//Функция возвращяет PId по имени процесса
DWORD GetMyProcessId(WCHAR* name);

//Функция вычисляет запущен ли процесс по имени
bool IsProcessRun(const WCHAR * const processName);

//Убить процесс, если запущен
void killProcess(const WCHAR * const processName);