#pragma once
#include <Windows.h>
#include <tlhelp32.h>

// ��������� ��� ������ ��������
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

//������� ���������� hwnd �� PId
HWND GetMainWindow(DWORD PId);

//������� ���������� PId �� ����� ��������
DWORD GetMyProcessId(WCHAR* name);

//������� ��������� ������� �� ������� �� �����
bool IsProcessRun(const WCHAR * const processName);

//����� �������, ���� �������
void killProcess(const WCHAR * const processName);