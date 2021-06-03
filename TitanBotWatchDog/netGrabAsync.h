#pragma once
#include "netGrab.h"
#include "CAutoMutex.h"
#include <string>
#include <thread>

// ������������� ����������� � ��������� ������
static CAutoMutex g_mutex;
// ����� ������
static std::string g_address;
static std::string g_anwser;
static int g_timeout;

void runGrabAsync(std::string* _address, int _timeout);

void threadGrabAsync(void);

//�������� ��������� ������
bool anwserReceived(std::string* _out_anwser);