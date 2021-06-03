#include "gdRequestParser.h"

GDRequestParser::GDRequestParser(std::string requestLine) {
	this->line = requestLine;
	this->parse();
}

void GDRequestParser::parse(void) {
	std::string requestLine = this->line.c_str();
	size_t pos_and (0), pos_eq (0);
	std::string name (""), value ("");
	do
	{
		pos_eq = 0;
		pos_and = requestLine.find('&');
		//if (pos_and == requestLine.npos) break;
		name = requestLine.substr(pos_eq, pos_and);
		pos_eq = name.find('=');
		value.clear();
		if (pos_eq != name.npos) value = name.substr(pos_eq+1);
		name = name.substr(0, pos_eq);
		requestLine.erase(0, pos_and+1);
		this->params.insert_or_assign(name, value);
	} while (pos_and < requestLine.npos);
}

std::string GDRequestParser::get(std::string param) {
	std::string result = "";
	result = this->params[param];
	return result;
}

int GDRequestParser::getSize(std::string param) {
	int result = 0;
	result = this->params[param].size();
	return result;
}