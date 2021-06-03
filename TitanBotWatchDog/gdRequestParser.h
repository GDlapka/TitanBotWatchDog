#pragma once
#include <string>
#include <map>

class GDRequestParser
{
public:
	GDRequestParser(std::string requestLine);
	std::string line;
	std::string get(std::string param);
	int getSize(std::string param);
private:
	void parse(void);
	std::map<std::string, std::string> params;
};