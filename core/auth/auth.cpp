#include "Auth.h"
#include <core/auth/keyauth/auth.hpp>
#include <core/auth/keyauth/utils.hpp>

using namespace KeyAuth;

std::string name = _("distortion Loader").decrypt(); // App name
std::string ownerid = _("REDACTED_OWNER_ID").decrypt(); // Account ID
std::string version = _("1.0").decrypt(); // Application version. Used for automatic downloads see video here https://www.youtube.com/watch?v=kW195PLCBKs
std::string url = _("https://keyauth.win/api/1.3/").decrypt(); // change if using KeyAuth custom domains feature
std::string path = _("").decrypt(); // (OPTIONAL) see tutorial here https://www.youtube.com/watch?v=I9rxt821gMk&t=1s

api KeyAuthApp(name, ownerid, version, url, path);

bool auth::initialize()
{
	KeyAuthApp.init();
	if (!KeyAuthApp.response.success)
	{
		std::string msg = KeyAuthApp.response.message;
		MessageBoxA(NULL, msg.c_str(), _("Error").decrypt(), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool auth::login(std::string username, std::string password)
{
	KeyAuthApp.login(username, password);
	if (!KeyAuthApp.response.success)
	{
		std::string msg = KeyAuthApp.response.message;
		MessageBoxA(NULL, msg.c_str(), _("Error").decrypt(), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool auth::registr(std::string username, std::string password, std::string license)
{
	KeyAuthApp.regstr(username, password, license);
	if (!KeyAuthApp.response.success)
	{
		std::string msg = KeyAuthApp.response.message;
		MessageBoxA(NULL, msg.c_str(), _("Error").decrypt(), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool auth::extend(std::string username, std::string license)
{
	KeyAuthApp.upgrade(username, license);
	if (!KeyAuthApp.response.success)
	{
		std::string msg = KeyAuthApp.response.message;
		MessageBoxA(NULL, msg.c_str(), _("Error").decrypt(), MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}