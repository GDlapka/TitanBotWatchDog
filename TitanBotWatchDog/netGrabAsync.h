#pragma once
#include "netGrab.h"
#include "CAutoMutex.h"
#include <string>
#include <thread>

// автоматически создаваемый и удаляемый мютекс
static CAutoMutex g_mutex;
// общие данные
static std::string g_address;
static std::string g_anwser;
static int g_timeout;

void runGrabAsync(std::string* _address, int _timeout);

void threadGrabAsync(void);

//проверка получения ответа
bool anwserReceived(std::string* _out_anwser);