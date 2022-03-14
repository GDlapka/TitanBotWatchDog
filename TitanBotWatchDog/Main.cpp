#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "netGrabAsync.h"
#include "GDString2.h"
#include "timer.hpp"
#include <windowsx.h>
#include <ctime>
#include "resource.h"
#include <winspool.h>

#include <fstream>
#include <string>

#include "TcpServer/TcpServer.h"
#include <thread>
#include "gdRequestParser.h"
#include "TitanBotLog.h"
#include "fProcess.h"

#define WD_VERSION L"3.3"

//#define FIRST_TIMER 1
#define SHORT_TIMER 40000
#define LONG_TIMER 50000
#define MSG_TIMER 10000

#define HMENU_START 9000
#define HMENU_STOP 9001
#define HMENU_RESTART_LONG 9002
#define HMENU_SHELL 9003
#define HMENU_CONNECT 9004
#define HMENU_DEVICES 9005
#define HMENU_COM_PORT_CHOOSER 9006
#define HMENU_DEVICE_CHOOSER 9007
#define HMENU_PUSH 9008
#define HMENU_PULL 9009
#define HMENU_RESTART_SHORT 9010
#define HMENU_LAUNCHER 9011
#define HMENU_KILLER 9012
#define HMENU_CLOSEGAME 9013
#define HMENU_RESTART_ROUTER 9015
#define HMENU_CHARGE 9016
#define HMENU_POWER 9017
#define HMENU_HOME 9018
#define HMENU_BACK 9019
#define HMENU_RECENT 9020
#define HMENU_KILLADB 9021
#define HMENU_DEVICE_EDIT 9022
#define HMENU_COM_UPDATE 9023
#define HMENU_DEVICES_UPDATE 9024

//User messages
#define WM_REQUEST_READY WM_USER + 101
#define WM_DEVICESREQUEST_READY WM_USER + 102

//UINT_PTR nTimerID = NULL;
bool isReseting = false;
bool isLongTimer = false;
bool isTimerOn = true;
time_t iPhoneRestartedLastTime = time(nullptr);
int lastDevNum = 0;

bool isShiftPressed = false;

const WCHAR g_szClassName[] = L"TBWatchDogWindowClass";

string wdRequest;
#if defined _DEBUG
	string titanBotWeb = "http://titanbot.online/wd/getCommand.php";
	string titanBotLog = "http://titanbot.online/log/?status=";
	string titanBotPackage = "http://titanbot.online/log/?package=1&status=";
	string titanBotWdDevices = "http://titanbot.online/wd/getCommand.php?devices=1";
#else
	string titanBotWeb = "https://titanbot.ru/wd/getCommand.php";
	string titanBotLog = "https://titanbot.ru/log/?status=";
	string titanBotPackage = "https://titanbot.ru/log/?package=1&status=";
	string titanBotWdDevices = "https://titanbot.ru/wd/getCommand.php?devices=1";
#endif

HWND hwnd;
HWND hStart;
HWND hStop;
HWND hRestartLong;
HWND hShell;
HWND hConnect;
HWND hDevices;
HWND hCOMPortChooser;
HWND hDeviceChooser;
HWND hDeviceEdit;
HWND hPush;
HWND hPull;
HWND hRestartShort;
HWND hLauncher;
HWND hKiller;
HWND hCloseGame;
HWND hRestartRouter;
HWND hCharge;
HWND hPower;
HWND hHome;
HWND hBack;
HWND hRecent;
HWND hKillADB;
HWND hCOMUpdate;
HWND hDevicesUpdate;
HWND hTotalRequests;
HWND hInRequests;
HWND hOutRequests = nullptr;

TcpServer* webServer = nullptr;

HBITMAP hBMCharge;
HBITMAP hBMPower;
HBITMAP hBMBack;
HBITMAP hBMHome;
HBITMAP hBMRecent;

wstring comPort = L"";

Timer watchdogTimer;
Timer logTimer;

//Log Class init
TBLog tbLog{ titanBotPackage };

void threadLauncher(void (*func)(void)) {
	std::thread Thread{ func };
	Thread.detach();
}
void threadLauncher(void(*func)(std::string), string strParam) {
	std::thread Thread{ func, strParam };
	Thread.detach();
}
void threadLauncher(void(*func)(std::string, bool), std::string  str, bool b) {
	std::thread Thread{ func, str, b };
	Thread.detach();
}

//Парсер ip в std::string
std::string getHostStr(const TcpServer::Client& client) {
	uint32_t ip = client.getHost();
	return std::string() + std::to_string(int(reinterpret_cast<char*>(&ip)[0])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[1])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[2])) + '.' +
		std::to_string(int(reinterpret_cast<char*>(&ip)[3])) + ':' +
		std::to_string(client.getPort());
}

void showCounter(void) {
	WCHAR wbuf[MAX_PATH] = L"";
	_itow_s(NetGrabCounter::getCount(), wbuf, MAX_PATH, 10);
	SetWindowText(hTotalRequests, wbuf);
	_itow_s(tbLog.getCount(true), wbuf, MAX_PATH, 10);
	SetWindowText(hInRequests, wbuf);
	_itow_s(tbLog.getCount(false), wbuf, MAX_PATH, 10);
	SetWindowText(hOutRequests, wbuf);
}

void launchWebServer(void) {
	//Создание объекта TcpServer с передачей аргументами порта и лямбда-фунции для обработки клиента
	TcpServer server(80, 
		[](TcpServer::Client client) {

		//Вывод адреса подключившего клиента в консоль
		//std::cout << "Connected host:" << getHostStr(client) << std::endl;

		//Ожидание данных от клиента
		int size = 0;
		while (!(size = client.loadData()));
		
		std::string request = client.getData();

		size_t pos = request.find('?');

		if (pos != request.npos) {
			request = request.substr(++pos);
			pos = request.find(' ');
			if (pos != request.npos) request = request.substr(0, pos);
			else request.erase();
		}
		else request.erase();

		//парсер параметров запроса
		GDRequestParser parser(request);
		std::string status = parser.get("status");

		client.sendData("ok", 2);

		tbLog.add(status);

		showCounter(); //для тестов
	}

	);

	//Запуск серевера
	if (server.start() == TcpServer::status::up) {
		//Если сервер запущен вывести сообщение и войти в поток ожиданий клиентов
		//std::cout << "Server is up!" << std::endl << std::endl;
		server.joinLoop();
		webServer = &server;
	}
	else {
		//Если сервер не запущен вывод кода ошибки и завершение программы
		//std::cout << "Server start error! Error code:" << int(server.getStatus()) << std::endl;
		MessageBox(hwnd, L"Ошибка запуска сервера!", L"Ошибка", MB_OK);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return;
	}

}

wstring getPortFile(void) {
	wifstream in(L"port.txt"); // окрываем файл для чтения
	wstring line = L"";
	if (in.is_open()) {
		getline(in, line);
		in.close();
	}
	return line;
}


void setPortFile(void) {
	wofstream out;          // поток для записи
	out.open(L"port.txt"); // окрываем файл для записи
	if (out.is_open()) {
		WCHAR buf[40];
		int index = ComboBox_GetCurSel(hCOMPortChooser);
		ComboBox_GetLBText(hCOMPortChooser, index, buf);
		out << buf;
	}
}

string devNum2Str(int devNum) {
	char buf[10] = "";
	_itoa_s(devNum, buf, 10, 10);
	return buf;
}

void comInit(void) {
	ComboBox_ResetContent(hCOMPortChooser);
	wstring fileLine = getPortFile();

	DWORD bytesNeeded, returned, i;
	PORT_INFO_1* pPortInfo1;
	wstring strNamePort;
	BYTE* pBuffer;

	// Определяем размер нужного нам буфера...
	if (!EnumPorts(NULL, 1, NULL, 0, &bytesNeeded, &returned) && GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
		pBuffer = new BYTE[bytesNeeded];
		// Получаем список имен портов
		EnumPorts(NULL, 1, pBuffer, bytesNeeded, &bytesNeeded, &returned);
		// Заполняем combo именами всех подходящих портов
		for (i = 0; i < returned; i++) {
			pPortInfo1 = (PORT_INFO_1*)(pBuffer + i * sizeof(PORT_INFO_1));
			strNamePort = pPortInfo1->pName;
			size_t pos = strNamePort.find(L"COM");
			if (pos != strNamePort.npos) {
				if (!strNamePort.empty()) strNamePort.erase(strNamePort.size() - 1); //erases ':' in the end of port
				ComboBox_AddString(hCOMPortChooser, strNamePort.c_str());
			}
		}
		delete pBuffer;
	}

	int pos = ComboBox_FindStringExact(hCOMPortChooser, -1, fileLine.c_str());
	if (pos == -1) {
		//no matches
		ComboBox_AddString(hCOMPortChooser, fileLine.c_str());
		ComboBox_SetCurSel(hCOMPortChooser, ComboBox_GetCount(hCOMPortChooser));
	}
	else {
		//matches were found
		ComboBox_SetCurSel(hCOMPortChooser, pos);
	}
}

bool send2COM(string devStr) {

	WCHAR buf[40];
	HANDLE hSerial; //handle of com port


	string msg = "";
	msg.append("\xD");
	msg.append("dev1 port");
	msg += devStr;
	msg.append("\xD");

	int index = ComboBox_GetCurSel(hCOMPortChooser);
	ComboBox_GetLBText(hCOMPortChooser, index, buf);

	wstring port = buf;
	port = L"\\\\.\\" + port;

	LPCWSTR sPortName = port.c_str();

	hSerial = ::CreateFile(sPortName, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (hSerial == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			MessageBox(NULL, L"Serial port does not exist.", L"ERROR", MB_OK);
		}
		MessageBox(NULL, L"Some other error occurred.", L"ERROR", MB_OK);
	}


	DCB dcbSerialParams;
	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
	memset(&dcbSerialParams, 0, sizeof(DCB));

	if (!GetCommState(hSerial, &dcbSerialParams))
	{
		MessageBox(NULL, L"Getting state error.", L"ERROR", MB_OK);
	}
	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	if (!SetCommState(hSerial, &dcbSerialParams))
	{
		MessageBox(NULL, L"Error setting serial port state.", L"ERROR", MB_OK);
	}
	//char data[] = {0xD, 'd', 'e', 'v', '1', ' ', 'p', 'o', 'r', 't', '5', '3', 0xD };
	//DWORD dwSize = sizeof(data);
	DWORD dwSize = msg.size() * sizeof(char);
	DWORD dwBytesWritten;

	BOOL iRet = WriteFile(hSerial, msg.c_str(), dwSize, &dwBytesWritten, NULL);

	//cout << dwSize << " Bytes in string. " << dwBytesWritten << " Bytes sended. " << endl;

	if (hSerial != NULL) CloseHandle(hSerial);

	return true;
}

bool send2COM(int devNum){
	if (devNum <= 0) return false;
	return send2COM(devNum2Str(devNum));
}

void stopWD(void) {
	isTimerOn = false;
	isReseting = false;
	isLongTimer = false;
	EnableWindow(hStart, true);
	EnableWindow(hStop, false);
}

void startWD() {
	isTimerOn = true;
	EnableWindow(hStart, false);
	EnableWindow(hStop, true);
}

bool setUSBPort(string port) {
	if (port.size() < 2) return false;
	port = port.substr(0, 2);
	int index = ComboBox_FindStringExact(hDeviceChooser, -1, (str2wide(port)).c_str());
	if (index < 0) return false;
	ComboBox_SetCurSel(hDeviceChooser, index);
	stopWD();
	//send2COM();
	return true;
}

void sendToLog(string mes) {
	string addr = titanBotLog + "%23%23%23%23_" + mes;
	netGrabAsync(&addr);
}


int getDeviceNumber(void) {
	int index = -1;
	WCHAR buf[10] = L"";

	GetWindowText(hDeviceEdit, buf, 10);
	if (wcsnlen_s(buf, 10) == 0) {
		//Нет ввода в hDeviceEdit
		if (ComboBox_GetCurSel(hDeviceChooser) == CB_ERR) {
			MessageBox(NULL, L"Please select or enter device number!", L"Warning", MB_OK);
			return 0;
		}
		index = ComboBox_GetCurSel(hDeviceChooser);
		ComboBox_GetLBText(hDeviceChooser, index, buf);
	}
	return _wtoi(buf);
}

void commander(string command, bool async = true) {
	bool ito = isTimerOn;
	if (async) ito = false;
	else stopWD();
	system(command.c_str());
	if (ito) startWD();
}

void deviceCommander(string command_prefix, string command_suffix, bool async) {
	int devNum = getDeviceNumber();
	if (devNum < 2) return;
	char buf[10] = "";
	_itoa_s(devNum, buf, 10, 10);
	string com1 = (command_prefix + buf + command_suffix);
	if (!async)	commander(com1, async);
	else threadLauncher(commander, com1, true);
}

void deviceCommander(string command_prefix) {
	deviceCommander(command_prefix, "", true);
}

void deviceCommander(string command_prefix, string command_suffix) {
	deviceCommander(command_prefix, command_suffix, true);
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

void keyEventHome(string device) {
	string command;
	command = (string)"c:\\tools\\starter.bat" + " " + device + " keyhome"; // Рестарт скрипта
	system(command.c_str());
	isReseting = true;
}

void restartAndroidShort(string device) {
	string command;
	//if (device == "51") command = (string)"c:\\tools\\start_launcher.bat" + " " + device;
	//else 
	//command = (string)"c:\\tools\\start_launcher_byWiFi.bat" + " " + device; // Полный Ребут С рестартом скрипта
	command = (string)"c:\\tools\\starter.bat" + " " + device + " relaunch"; // Рестарт скрипта
	system(command.c_str());
	isReseting = true;
}

void restartAndroidLong(string device) {
	string command;
	command = (string)"c:\\tools\\start_launcher_byWiFi.bat" + " " + device; // Полный Ребут
	system(command.c_str());
	isReseting = true;
}

void sbReload(string device) {
	string command;
	command = (string)"c:\\tools\\ssh_run.bat" + " " + device;
	system(command.c_str());
	isReseting = true;
}

void rebootLauncher(void) {
	bool wasStarted;
	if (wasStarted = isTimerOn) stopWD();
	int devNum = getDeviceNumber();
	if (devNum <= 0) return;
	if (devNum < 17) {
		send2COM(devNum);
		return;
	} 
	deviceCommander((string)"c:\\tools\\start_launcher_byWiFi.bat ");
	if (wasStarted) startWD();
}

void getStatus(void) {
	wdRequest.clear();
	netGrabAsync(&titanBotWeb, &wdRequest, hwnd, WM_REQUEST_READY);
}

void parseWDRequest(void) {
	bool timerValue = isTimerOn;
	isTimerOn = false;
	if (wdRequest[0] != 'o') {
		//alert!
		string device = wdRequest.substr(0, 2);
		char comType = '!';
		if (wdRequest.size() >= 3) comType = wdRequest.at(2);

		int devNum = 0;

		devNum = atoi(device.c_str());

		if (devNum > 0 && devNum <= 16) {
			if (lastDevNum == devNum && time(nullptr) - iPhoneRestartedLastTime < 300)
				send2COM(device);
			sbReload(device);
			send2COM(device);
			iPhoneRestartedLastTime = time(nullptr);
			lastDevNum = devNum;
			sendToLog((string)"WatchDog_Fixing_device_" + device + ".");
		}

		if (devNum >= 31 && devNum < 100) {

			if (comType == '!') {
				//wstring wReq = str2wide(device);
				//SetWindowText(hwnd, wReq.c_str());
				sendToLog((string)"WatchDog_Fixing_device_" + device + ".");
				threadLauncher(restartAndroidShort, device);
			}
			else if (comType == '0') {
				//fullRestart
				sendToLog((string)"WatchDog_restart_LONG_device_" + device + ".");
				threadLauncher(restartAndroidLong, device);
			}
			else if (comType == '1') {
				//shortrestart
				sendToLog((string)"WatchDog_restart_SHORT_device_" + device + ".");
				threadLauncher(restartAndroidShort, device);
			}
			else if (comType == '2') {
				//keyback
				threadLauncher(keyEventHome, device);
			}
		}
	}
	isTimerOn = timerValue;
}

void timeToCheck(void) {
	if (isReseting) {
		watchdogTimer.add(std::chrono::milliseconds(LONG_TIMER), timeToCheck, true);
		isReseting = false;
		isLongTimer = true;
		return;
	}
	else {
		watchdogTimer.add(std::chrono::milliseconds(SHORT_TIMER), timeToCheck, true);
		if (isLongTimer) isLongTimer = false;
		if (isTimerOn) getStatus();
	}
}

void timeToSendLog(void) {
	tbLog.send();
	logTimer.add(std::chrono::milliseconds(MSG_TIMER), timeToSendLog, true);
}

void showControls(int nCmdShow) {
	ShowWindow(hStart, nCmdShow);
	ShowWindow(hStop, nCmdShow);
	ShowWindow(hRestartLong, nCmdShow);
	ShowWindow(hShell, nCmdShow);
	ShowWindow(hConnect, nCmdShow);
	ShowWindow(hDevices, nCmdShow);
	ShowWindow(hPush, nCmdShow);
	ShowWindow(hPull, nCmdShow);
	ShowWindow(hCOMPortChooser, nCmdShow);
	ShowWindow(hDeviceChooser, nCmdShow);

	if (nCmdShow == SW_SHOW) {
		ShowWindow(hwnd, nCmdShow);
		UpdateWindow(hwnd);
	}
}

void deviceInit(void) {
	/*
	//получение списка устройств из файла devices.txt
	wifstream in(L"devices.txt"); // окрываем файл для чтения
	wstring line = L"";
	if (in.is_open()) {
	WCHAR buf;
	while (getline(in, line)) {
	ComboBox_AddString(hDeviceChooser, (line + L"\0").c_str());
	}
	in.close();
	}*/
	string request;
	for (int i = 0; request.empty() && i < 10; i++) netGrabSpace(&titanBotWdDevices, &request);
	
	if (request.empty()) {
		MessageBox(hwnd, L"Couldn't get device list! Server is unreachable.", L"ERROR", MB_ICONERROR);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
	} else {
		//очищаем combo
		ComboBox_ResetContent(hDeviceChooser);
		wstring wrequest = str2wide(request);
		size_t pos = 0;
		do {
			if (pos != wrequest.npos && pos + 1 != wrequest.npos && pos != 0) wrequest.erase(0, pos + 1);
			pos = wrequest.find(L'<');
			if (pos != 0) 
				ComboBox_AddString(hDeviceChooser, (wrequest.substr(0, pos) + L"\0").c_str());
			pos = wrequest.find(L'>');
		} while (pos != wrequest.npos);

		watchdogTimer.add(std::chrono::milliseconds(SHORT_TIMER), timeToCheck, true);
		logTimer.add(std::chrono::milliseconds(MSG_TIMER), timeToSendLog, true);
		showControls(SW_SHOW);
	}

	//ComboBox_AddString(hDeviceChooser, L"01\0");
}

// Step 4: the Window Procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WORD wmId = LOWORD(wParam);
	WORD wmEvent = HIWORD(wParam);

	int index = -1;
	WCHAR buf[10] = L"";
	wstring bufStr = L"";
	int devNum = -1;
	string command = "c:\\tools\\start_launcher.bat";
	
	switch (msg)
	{
	case WM_REQUEST_READY:
		parseWDRequest();
		//MessageBox(NULL, (str2wide(wdRequest)).c_str(), L"Request Captured!", MB_OK); //debug request
		break;
	case WM_COMMAND:
		if (wmEvent == CBN_SELCHANGE) {
			if (wmId == HMENU_COM_PORT_CHOOSER) {
				//COM combo
				if (ComboBox_GetCurSel(hCOMPortChooser) != CB_ERR) { //Ввод есть
					setPortFile();
				}
			}
			else {
				//Device combo
				if (ComboBox_GetCurSel(hDeviceChooser) != CB_ERR) { //Ввод есть
					SetWindowText(hDeviceEdit, L"");
				}
			}
		}
		switch (wmId)
		{
		case HMENU_COM_UPDATE:
			threadLauncher(comInit);
			break;
		case HMENU_DEVICES_UPDATE:
			threadLauncher(deviceInit);
			break;
		case HMENU_START:
			startWD();
			break;
		case HMENU_STOP:
			stopWD();
			break;
		case HMENU_RESTART_LONG:
			threadLauncher(rebootLauncher);
			break;
		case HMENU_SHELL:
			deviceCommander("c:\\tools\\shell.bat ", "", true);
			break;
		case HMENU_DEVICES:
			threadLauncher(showDevices);
			break;
		case HMENU_CONNECT:
			deviceCommander("c:\\tools\\reconnect.bat ");
			break;
		case HMENU_PUSH:
			deviceCommander("c:\\tools\\push.bat ", " 1");
			break;
		case HMENU_PULL:
			if (HIWORD(GetAsyncKeyState(VK_SHIFT))) deviceCommander("c:\\tools\\pull.bat ", " 1"); 
			else deviceCommander("c:\\tools\\pullRun.bat ", " 1");
			break;
		case HMENU_RESTART_SHORT:
			deviceCommander("c:\\tools\\starter.bat ", " relaunch");
			break;
		case HMENU_LAUNCHER:
			deviceCommander("c:\\tools\\starter.bat ", " launcher connected");
			break;
		case HMENU_KILLER:
			deviceCommander("c:\\tools\\starter.bat ", " killer connected");
			break;
		case HMENU_CLOSEGAME:
			deviceCommander("c:\\tools\\starter.bat ", " closer connected");
			break;
		case HMENU_KILLADB:
			commander("c:\\tools\\adb.exe kill-server ", false);
			break;
		case HMENU_CHARGE:
			deviceCommander("c:\\tools\\starter.bat ", " charge connected");
			break;
		case HMENU_POWER:
			deviceCommander("c:\\tools\\starter.bat ", " keypower connected");
			break;
		case HMENU_BACK:
			deviceCommander("c:\\tools\\starter.bat ", " keyback connected");
			break;
		case HMENU_HOME:
			deviceCommander("c:\\tools\\starter.bat ", " keyhome connected");
			break;
		case HMENU_RECENT:
			deviceCommander("c:\\tools\\starter.bat ", " keyrecent connected");
			break;
		case HMENU_RESTART_ROUTER:
			break;
		default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		break;
	case WM_CLOSE:
		if (webServer != nullptr) webServer->stop();
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

	wchar_t name[100] = L"TitanBot WatchDog v";
	wcscat_s(name, (wchar_t*)WD_VERSION);
	// Step 2: Creating the Window
	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		name,
		WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 10, 330, 530,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, L"Window Creation Failed!", L"Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}
	
	hStart = CreateWindow(L"BUTTON", L"WatchDog Start", WS_VISIBLE | WS_CHILD, 10, 10, 140, 60, hwnd, (HMENU)HMENU_START, NULL, NULL);
	hStop = CreateWindow(L"BUTTON", L"WatchDog STOP", WS_VISIBLE | WS_CHILD, 160, 10, 140, 60, hwnd, (HMENU)HMENU_STOP, NULL, NULL);
	hRestartLong = CreateWindow(L"BUTTON", L"RESTART Long", WS_VISIBLE | WS_CHILD, 160, 80, 140, 60, hwnd, (HMENU)HMENU_RESTART_LONG, NULL, NULL);
	hConnect = CreateWindow(L"BUTTON", L"Connect", WS_VISIBLE | WS_CHILD, 10, 330, 140, 60, hwnd, (HMENU)HMENU_CONNECT, NULL, NULL);
	hDevices = CreateWindow(L"BUTTON", L"Devices", WS_VISIBLE | WS_CHILD, 160, 330, 140, 60, hwnd, (HMENU)HMENU_DEVICES, NULL, NULL);
	hKillADB = CreateWindow(L"BUTTON", L"Kill ADB", WS_VISIBLE | WS_CHILD, 10, 290, 140, 30, hwnd, (HMENU)HMENU_KILLADB, NULL, NULL);
	hCharge = CreateWindowEx(0, L"BUTTON", L"", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_BITMAP, 150, 290, 30, 30, hwnd, (HMENU)HMENU_CHARGE, NULL, NULL);
	hPower = CreateWindowEx(0, L"BUTTON", L"", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_BITMAP, 180, 290, 30, 30, hwnd, (HMENU)HMENU_POWER, NULL, NULL);
	hBack = CreateWindowEx(0, L"BUTTON", L"", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_BITMAP, 210, 290, 30, 30, hwnd, (HMENU)HMENU_BACK, NULL, NULL);
	hHome = CreateWindowEx(0, L"BUTTON", L"", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_BITMAP, 240, 290, 30, 30, hwnd, (HMENU)HMENU_HOME, NULL, NULL);
	hRecent = CreateWindowEx(0, L"BUTTON", L"", BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_BITMAP, 270, 290, 30, 30, hwnd, (HMENU)HMENU_RECENT, NULL, NULL);
	hPush = CreateWindow(L"BUTTON", L"Push scripts", WS_VISIBLE | WS_CHILD, 10, 400, 140, 30, hwnd, (HMENU)HMENU_PUSH, NULL, NULL);
	hPull = CreateWindow(L"BUTTON", L"PULL scripts", WS_VISIBLE | WS_CHILD, 160, 400, 140, 30, hwnd, (HMENU)HMENU_PULL, NULL, NULL);
	hCOMPortChooser = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_DISABLENOSCROLL | WS_VSCROLL, 70, 440, 80, 180, hwnd, (HMENU)HMENU_COM_PORT_CHOOSER, NULL, NULL);
	hDeviceChooser = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | CBS_DROPDOWNLIST | CBS_HASSTRINGS | CBS_DISABLENOSCROLL | WS_VSCROLL, 250, 440, 50, 180, hwnd, (HMENU)HMENU_DEVICE_CHOOSER, NULL, NULL);
	hDeviceEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 441, 25, 20, hwnd, (HMENU)HMENU_DEVICE_EDIT, NULL, NULL);
	hRestartShort = CreateWindow(L"BUTTON", L"RESTART Short", WS_VISIBLE | WS_CHILD, 10, 80, 140, 60, hwnd, (HMENU)HMENU_RESTART_SHORT, NULL, NULL);
	hLauncher = CreateWindow(L"BUTTON", L"Start Bot", WS_VISIBLE | WS_CHILD, 10, 150, 140, 60, hwnd, (HMENU)HMENU_LAUNCHER, NULL, NULL);
	hKiller = CreateWindow(L"BUTTON", L"Kill Bot", WS_VISIBLE | WS_CHILD, 160, 150, 140, 60, hwnd, (HMENU)HMENU_KILLER, NULL, NULL);
	hCloseGame = CreateWindow(L"BUTTON", L"Close Game", WS_VISIBLE | WS_CHILD, 10, 220, 140, 60, hwnd, (HMENU)HMENU_CLOSEGAME, NULL, NULL);
	hShell = CreateWindow(L"BUTTON", L"Shell", WS_VISIBLE | WS_CHILD, 160, 220, 140, 60, hwnd, (HMENU)HMENU_SHELL, NULL, NULL);
	//hRestartRouter = CreateWindow(L"BUTTON", L"Restart Router", WS_VISIBLE | WS_CHILD, 160, 80, 140, 30, hwnd, (HMENU)HMENU_RESTART_ROUTER, NULL, NULL);

	hBMCharge = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, 0);
	hBMPower = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, 0);
	hBMBack = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BITMAP4), IMAGE_BITMAP, 0, 0, 0);
	hBMHome = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BITMAP3), IMAGE_BITMAP, 0, 0, 0);
	hBMRecent = (HBITMAP)LoadImage(hInstance, MAKEINTRESOURCE(IDB_BITMAP5), IMAGE_BITMAP, 0, 0, 0);
	SendMessage(hCharge, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBMCharge);
	SendMessage(hPower, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBMPower);
	SendMessage(hHome, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBMHome);
	SendMessage(hBack, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBMBack);
	SendMessage(hRecent, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBMRecent);

	EnableWindow(hStart, false);

	showControls(SW_HIDE);

	threadLauncher(comInit);
	threadLauncher(deviceInit);

	CreateWindow(L"STATIC", L"Total:", WS_VISIBLE | WS_CHILD, 10, 467, 35, 20, hwnd, (HMENU)0, NULL, NULL);
	CreateWindow(L"STATIC", L"in:", WS_VISIBLE | WS_CHILD, 130, 467, 25, 20, hwnd, (HMENU)0, NULL, NULL);
	CreateWindow(L"STATIC", L"out:", WS_VISIBLE | WS_CHILD, 222, 467, 30, 20, hwnd, (HMENU)0, NULL, NULL);
	hTotalRequests = CreateWindow(L"STATIC", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER, 50, 466, 50, 20, hwnd, (HMENU)0, NULL, NULL);
	hInRequests = CreateWindow(L"STATIC", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER, 150, 466, 50, 20, hwnd, (HMENU)0, NULL, NULL);
	hOutRequests = CreateWindow(L"STATIC", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER, 250, 466, 50, 20, hwnd, (HMENU)0, NULL, NULL);

	hCOMUpdate = CreateWindow(L"BUTTON", L"COM:", WS_VISIBLE | WS_CHILD | WS_BORDER, 10, 441, 50, 20, hwnd, (HMENU)HMENU_COM_UPDATE, NULL, NULL);
	hDevicesUpdate = CreateWindow(L"BUTTON", L"Device:", WS_VISIBLE | WS_CHILD | WS_BORDER, 160, 441, 55, 20, hwnd, (HMENU)HMENU_DEVICES_UPDATE, NULL, NULL);

	killProcess(L"adb.exe"); //если adb висит - сервер не заведется

	std::thread WebServerThread {launchWebServer};
	WebServerThread.detach();
	
	// Step 3: The Message Loop
	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		if (Msg.message == WM_KEYDOWN) {
			if (Msg.hwnd == hDeviceEdit) {
				if (Msg.wParam != 0x30 && Msg.wParam != 0x31 && Msg.wParam != 0x32 && Msg.wParam != 0x33 && Msg.wParam != 0x34
					&& Msg.wParam != 0x35 && Msg.wParam != 0x36 && Msg.wParam != 0x37 && Msg.wParam != 0x38 && Msg.wParam != 0x39
					&& Msg.wParam != VK_BACK && Msg.wParam != VK_RETURN && Msg.wParam != VK_DELETE
					&& Msg.wParam != VK_LEFT && Msg.wParam != VK_RIGHT && Msg.wParam != 0xBE && Msg.wParam != 191 && Msg.wParam != 110
					&& Msg.wParam != 0x60 && Msg.wParam != 0x61 && Msg.wParam != 0x62 && Msg.wParam != 0x63 && Msg.wParam != 0x64 && Msg.wParam != 0x65
					&& Msg.wParam != 0x66 && Msg.wParam != 0x67 && Msg.wParam != 0x68 && Msg.wParam != 0x69 && Msg.wParam != 0x6E)
				{
					Msg.wParam = 0;
				}
				ComboBox_SetCurSel(hDeviceChooser, -1);
			}
		}
		
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
	return Msg.wParam;
}