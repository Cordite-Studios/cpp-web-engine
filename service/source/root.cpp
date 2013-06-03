
#include "handler.hpp"
#include "handler_register.hpp"
#include "root.hpp"
namespace Cordite {
//---------------------------
R_Authorization RootHandler::authorize(const std::string &, R_SessionVersion)
{
	return RA_NULL;
}
const char * RootHandler::handle()
{
	return "";
}
const bool RootHandler::isRegex()
{
	return false;
}
static HandlerRegister<RootHandler> rootRegister;
//---------------------------
};