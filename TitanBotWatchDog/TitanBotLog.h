#pragma once
#include <string>
#include <queue>
#include <ctime>
#include "netGrab.h"

struct LogLine
{
	time_t timestamp{ time(0) };
	std::string status{""};
	//int fuel{ -1 };
	//int charge{101};
	//std::string msg{""};
};

class TBLog
{
public:
	TBLog(std::string addressBeforeStatusField);
	void add(std::string status);
	std::queue<LogLine> log;
	void send(void);
	int getCount(bool _true_is_IN_false_is_OUT);
private:
	std::string address;
	int inRequestsCounter;
	int outRequestsCounter;
	std::string package;
};