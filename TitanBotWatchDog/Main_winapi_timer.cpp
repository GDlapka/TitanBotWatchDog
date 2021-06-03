#include <windows.h>
#include "netGrabAsync.h"
#include "GDString2.h"

#include <fstream>
#include <string.h>

#define FIRST_TIMER 1
#define SHORT_TIMER 20000
#define LONG_TIMER 60000

#define HMENU_START 9000
#define HMENU_STOP 9001
#define HMENU_LAUNCH 9002
#define HMENU_REBOOT 9003
#define HMENU_UNLOCK 9004
#define HMENU_DEVICES 9005


UINT_PTR nTimerID = NULL;
bool isReseting = false;
bool isLongTimer = false;

const WCHAR g_szClassName[] = L"myWindowClass";

string request = "ok";
string titanBotWeb = "http://titanbot.ru/wd/index.php";
string titanBotLog = "http://titanbot.ru/log/?status=";

HWND hwnd;
HWND hStart;
HWND hStop;
HWND hLauncher;
HWND hReboot;
HWND hUnlock;
HWND hDevices;

void killUserTimer(HWND hWnd, UINT_PTR uIDEvent) {
	KillTimer(hWnd, uIDEvent);
	nTimerID = NULL;
}

void stopWD(void) {
	killUserTimer(hwnd, FIRST_TIMER);
	isReseting = false;
	isLongTimer = false;
	EnableWindow(hStart, true);
	EnableWindow(hStop, false);
}

void showDevices(void) {
	char* tmp_name = _tempnam(NULL, NULL);
	auto cmd_name = std::string(tmp_name) + ".cmd";
	{
	std::ofstream f(cmd_name);
	f << "c:\\tools\\adb devices\n";
	f << "timeout \/t 5\n";
	f << "Exit \/B\n";
	}
	int result = std::system(cmd_name.c_str());
	std::remove(cmd_name.c_str());
	free(tmp_name);
}

void rebootLauncher(void) {
	/*char* tmp_name = _tempnam(NULL, NULL);
	auto cmd_name = std::string(tmp_name) + ".cmd";
	{
		std::ofstream f(cmd_name);
		f << "@echo off\n";
		f << "tasklist \/v \/fi \"IMAGENAME eq cmd.exe\"\|findstr \"mytitle\" \>Nul \|\| (\n";
		f << "start \"mytitle\" \"C:\\tools\\reboot.bat\"\n";
		f << "timeout \/t 8\n";
		f << ")\n";
		f << "for \/F \"tokens = 1, 2 * delims = \" %%A in (\'tasklist \/v \/fi \"IMAGENAME eq cmd.exe\"\') do (echo:%%C\|findstr \"mytitle\" \>Nul \&\& set \"pid = %%B\")\n";
		f << "taskkill \/PID %pid%\n";
		f << "Exit \/B\n";
	}
	int result = std::system(cmd_name.c_str());
	std::remove(cmd_name.c_str());
	free(tmp_name);*/
	system("c:\\tools\\start_launcher.bat");
	isReseting = true;
}

void sendToLog(string mes) {
	request.clear();
	string addr = titanBotLog + "%23%23%23%23_" + mes;
	netGrabSpace(&addr, &request);
}

void getStatus(void) {
	request.clear();
	netGrabSpace(&titanBotWeb, &request);
	
	if (request[0] != 'o') {
		//alert!
		string device = request.substr(0, 2);

		if (device == "51") {
			//wstring wReq = str2wide(device);
			//SetWindowText(hwnd, wReq.c_str());
			sendToLog((string)"WatchDog_restarted_device_" + device + ".");
			rebootLauncher();
		}
	}
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WORD wmId = LOWORD(wParam);
	WORD wmEvent = HIWORD(wParam);
	switch (msg)
	{
	case WM_TIMER:
		//MessageBox(hwnd, L"", L"", MB_OK);
		if (isReseting) {
			killUserTimer(hwnd, FIRST_TIMER);
			nTimerID = SetTimer(hwnd, FIRST_TIMER, LONG_TIMER, NULL);
			isReseting = false;
			isLongTimer = true;
			break;
		}
		else if (isLongTimer) {
			killUserTimer(hwnd, FIRST_TIMER);
			nTimerID = SetTimer(hwnd, FIRST_TIMER, SHORT_TIMER, NULL);
			isLongTimer = false;
		}
		getStatus();
		break;
	case WM_COMMAND:
		switch (wmId)
		{
		case HMENU_START:
			//MessageBox(NULL, L"", L"", MB_OK);
			nTimerID = SetTimer(hwnd, FIRST_TIMER, SHORT_TIMER, NULL);
			EnableWindow(hStart, false);
			EnableWindow(hStop, true);
			break;
		case HMENU_STOP:
			stopWD();
			break;
		case HMENU_LAUNCH:
			stopWD();
			system("c:\\tools\\start_launcher.bat");
			break;
		case HMENU_REBOOT:
			stopWD();
			system("c:\\tools\\adb reboot");
			break;
		case HMENU_DEVICES:
			stopWD();
			showDevices();
			break;
		case HMENU_UNLOCK:
			stopWD();
			system("c:\\tools\\unlock.bat");
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_CLOSE:
		killUserTimer(hwnd, FIRST_TIMER);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	MSG Msg;

	//Step 1: Registering the Window Class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, L"Window Registration Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	// Step 2: Creating the Window
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		L"TitanBot WatchDog",
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 330, 260,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}


	hStart = CreateWindow(L"BUTTON", L"WatchDog Start", WS_VISIBLE | WS_CHILD, 10, 10, 140, 60, hwnd, (HMENU)HMENU_START, NULL, NULL);
	hStop = CreateWindow(L"BUTTON", L"WatchDog STOP", WS_VISIBLE | WS_CHILD, 160, 10, 140, 60, hwnd, (HMENU)HMENU_STOP, NULL, NULL);
	hLauncher = CreateWindow(L"BUTTON", L"launch.sh", WS_VISIBLE | WS_CHILD, 10, 80, 140, 60, hwnd, (HMENU)HMENU_LAUNCH, NULL, NULL);
	hReboot = CreateWindow(L"BUTTON", L"REBOOT", WS_VISIBLE | WS_CHILD, 160, 80, 140, 60, hwnd, (HMENU)HMENU_REBOOT, NULL, NULL);
	hUnlock = CreateWindow(L"BUTTON", L"Unlock", WS_VISIBLE | WS_CHILD, 10, 150, 140, 60, hwnd, (HMENU)HMENU_UNLOCK, NULL, NULL);
	hDevices = CreateWindow(L"BUTTON", L"Devices", WS_VISIBLE | WS_CHILD, 160, 150, 140, 60, hwnd, (HMENU)HMENU_DEVICES, NULL, NULL);

	EnableWindow(hStart, false);

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	nTimerID = SetTimer(hwnd, FIRST_TIMER, SHORT_TIMER, NULL);
	
	// Step 3: The Message Loop
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}