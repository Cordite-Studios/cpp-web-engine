#include <regex>
#include "handler.hpp"
#include "handler_register.hpp"

namespace Cordite {
//---------------------------

Handler::Response BaseHandler::get(Handler::Request &)
{
	Handler::Response res;
	res.meta.code = RSC_ERR_BAD_METHOD;
	return res;
}
Handler::Response BaseHandler::put(Handler::Request &)
{
	Handler::Response res;
	res.meta.code = RSC_ERR_BAD_METHOD;
	return res;
}
Handler::Response BaseHandler::post(Handler::Request &)
{
	Handler::Response res;
	res.meta.code = RSC_ERR_BAD_METHOD;
	return res;
}
Handler::Response BaseHandler::del(Handler::Request &)
{
	Handler::Response res;
	res.meta.code = RSC_ERR_BAD_METHOD;
	return res;
}
Handler::Response BaseHandler::options(R_Authorization)
{
	Handler::Response res;
	res.meta.code = RSC_SRV_ERR_NOT_IMPL;
	return res;
}
class NullHandler : public Handler {
	virtual const char * handle()
	{
		return "\t";
	}
	virtual const bool isRegex()
	{
		return false;
	}
	virtual Handler::Response get(Handler::Request &)
	{
		Handler::Response res;
		res.meta.code = RSC_ERR_NOT_FOUND;
		return res;
	}
	virtual Handler::Response put(Handler::Request &)
	{
		Handler::Response res;
		res.meta.code = RSC_ERR_NOT_FOUND;
		return res;
	}
	virtual Handler::Response post(Handler::Request &)
	{
		Handler::Response res;
		res.meta.code = RSC_ERR_NOT_FOUND;
		return res;
	}
	virtual Handler::Response del(Handler::Request &)
	{
		Handler::Response res;
		res.meta.code = RSC_ERR_NOT_FOUND;
		return res;
	}
	virtual Handler::Response options(R_Authorization)
	{
		Handler::Response res;
		res.meta.code = RSC_ERR_NOT_FOUND;
		return res;
	}
	virtual R_Authorization authorize(const std::string &, R_SessionVersion)
	{
		return RA_GUEST;
	}
	virtual void match(const std::string & part, Request & req)
	{
	}
};
static NullHandler nullHandle;


//---------------------------
Handler & Handler::resolve(const std::string & part, Request * req)
{
	if(part == "") return *this;
	for(auto h : hard_handles)
	{
		if(part.compare(h->handle()) == 0)
		{
			return *h;
		}
	}
	for(auto r : regex_handles)
	{
		std::regex reg(r->handle());
		if(std::regex_match(part,reg))
		{
			if(req)
			{
				r->match(part, *req);
			}
			return *r;
		}
	}
	return nullHandle;
}
void Handler::addHandler(Handler * h)
{
	if(h->isRegex())
	{
		regex_handles.push_back(h);
	}
	else
	{
		hard_handles.push_back(h);
	}
}
void BaseHandler::match(const std::string & part, Request & req)
{
	//Nothing.
}
/*Handler & Handler::getNullHandler()
{
	return NullHandler;
}*/

//---------------------------	
};