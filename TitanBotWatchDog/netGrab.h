#pragma once

#include <stdio.h>

#include "curl/curl.h"
#pragma comment(lib,"lib/libcurl.lib")

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <thread>
#include <string>
using namespace std;

#include <stdexcept>

#ifdef DLL_EXPORT
#define DLL_PREFIX __declspec(dllexport)
#else
#define DLL_PREFIX __declspec(dllimport)
#endif // DLL_EXPORT

static size_t curl_writer(char *ptr, size_t size, size_t nmemb, string* data);

DLL_PREFIX
void netGrabSpace(string* _address, string* _outfile);

DLL_PREFIX
void netGrabSpace(string* _address, string* _outfile, HWND _hwnd_to_send_Message, UINT _messageNumber);

DLL_PREFIX
void netGrabSpace(string* _address);

DLL_PREFIX
void netGrabAsync(string* _address, string* _outfile, HWND _hwnd_to_send_Message, UINT _messageNumber);

DLL_PREFIX
void netGrabAsync(string* _address);

DLL_PREFIX
void netGrabFile(string* resource, string* filename);

class NetGrabCounter
{
public:
	static int getCount(void) { return NetGrabCounter::counter; };
	NetGrabCounter& operator ++() { NetGrabCounter::counter++; return *this; };
private:
	static int counter;
};