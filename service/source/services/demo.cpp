#include "handler.hpp"
#include "handler_register.hpp"
#include "root.hpp"

namespace Cordite {
	namespace Services {
//---------------------------
class DemoHandler : public BaseHandler
{
public:
	virtual const char * handle() override
	{
		return "demo";
	}
	virtual const bool isRegex() override
	{
		return false;
	}
	DemoHandler() = default;
	virtual R_Authorization authorize(const std::string &, R_SessionVersion) override
	{
		return RA_SELF;
	}
	virtual Handler::Response get(Handler::Request &) override
	{
		Handler::Response res;
		res.meta.code = RSC_OK;
		res.body = "Hello World";
		return res;
	}
};
static HandlerRegister<DemoHandler, RootHandler> demoRegister;
class DemoHandler2 : public BaseHandler
{
public:
	virtual const char * handle() override
	{
		return ".*";
	}
	virtual const bool isRegex() override
	{
		return true;
	}
	DemoHandler2() = default;
	virtual R_Authorization authorize(const std::string &, R_SessionVersion)
	{
		return RA_SELF;
	}
	virtual Handler::Response get(Handler::Request & req) override
	{
		Handler::Response res;
		res.meta.code = RSC_OK;
		res.body = "Param: ";
		res.body += req.requestData["demo"];
		return res;
	}
	virtual void match(const std::string & part, Request & r) override
	{
		//Simple echo
		r.requestData["demo"] = part;
	}
};
static HandlerRegister<DemoHandler2, DemoHandler> demoRegister2;
class DemoHandler3 : public BaseHandler
{
public:
	virtual const char * handle() override
	{
		return "json";
	}
	virtual const bool isRegex() override
	{
		return false;
	}
	DemoHandler3() = default;
	virtual R_Authorization authorize(const std::string &, R_SessionVersion)
	{
		return RA_SELF;
	}
	virtual Handler::Response get(Handler::Request & req) override
	{
		Handler::Response res;
		res.meta.code = RSC_OK;
		res.body = "{\"hello\":\"world\"}";
		res.meta.type = RRT_JSON;
		return res;
	}
};
static HandlerRegister<DemoHandler3, DemoHandler> demoRegister3;
//---------------------------
	};
};