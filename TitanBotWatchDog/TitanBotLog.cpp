#include "TitanBotLog.h"

TBLog::TBLog(std::string addressBeforeStatusField) {
	this->inRequestsCounter = 0;
	this->outRequestsCounter = 0;
	this->address = addressBeforeStatusField;
}

void TBLog::add(std::string status){
	if (status.empty()) return;
	LogLine line;
	line.status = status;
	this->log.push(line);
	this->inRequestsCounter++;
}

void checkTwoSymbols(char * buf, int len) {
	if (len <= 3) return;
	if (strnlen_s(buf, len) != 1) return;
	buf[1] = buf[0];
	buf[0] = '0';
	buf[2] = '\0';
}

void TBLog::send(void) {
	if (this->log.empty()) return;
	LogLine line;
	tm timestamp;
	this->package.clear();
	int sendCounter = 0;
	while (!(this->log.empty()))
	{
		line = this->log.front();
		//gmtime_s(&timestamp, &(line.timestamp));
		localtime_s(&timestamp, &(line.timestamp));
		char buf[5] = "";
		_itoa_s(timestamp.tm_year + 1900, buf, 5, 10);
		this->package += std::string("[") + buf + "-";
		_itoa_s(timestamp.tm_mon + 1, buf, 5, 10); checkTwoSymbols(buf, 5); this->package += std::string(buf) + "-";
		_itoa_s(timestamp.tm_mday, buf, 5, 10); checkTwoSymbols(buf, 5); this->package += std::string(buf) + "%20";
		_itoa_s(timestamp.tm_hour, buf, 5, 10); checkTwoSymbols(buf, 5); this->package += std::string(buf) + ":";
		_itoa_s(timestamp.tm_min, buf, 5, 10); checkTwoSymbols(buf, 5);	this->package += std::string(buf) + ":";
		_itoa_s(timestamp.tm_sec, buf, 5, 10); checkTwoSymbols(buf, 5);	this->package += std::string(buf) + "]%20" + line.status;
		this->log.pop();
		sendCounter++;
		this->package += std::string("%0D%0A");
	}
	//sending
	this->package = this->address + this->package;
	netGrabAsync(&package);
	this->outRequestsCounter += sendCounter;
}

int TBLog::getCount(bool is_in) {
	if (is_in) return this->inRequestsCounter;
	return this->outRequestsCounter;
}