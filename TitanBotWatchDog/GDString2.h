#pragma once

#include <string>
#include <algorithm>

//�������������� � ascii � �������
std::string wide2str(std::wstring _src);

std::wstring str2wide(std::string _src);

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);