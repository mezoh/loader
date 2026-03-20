#pragma once
#include <iostream>

namespace auth
{
	bool initialize();
	bool login(std::string username, std::string password);
	bool registr(std::string username, std::string password, std::string license);
	bool extend(std::string username, std::string license);
}

