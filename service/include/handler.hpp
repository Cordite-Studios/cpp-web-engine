#pragma once
#include <string>
#include <list>
#include <map>
#include "authorization.hpp"
#include "response.hpp"
#include "response_type.hpp"
#include "session_version.hpp"

namespace Cordite {
	enum HandlerMethod {
		HM_NULL = 0,
		HM_GET,
		HM_POST,
		HM_PUT,
		HM_DELETE,
		HM_OPTIONS,
		HM_AUTH
	};
	class HandlerRequest {
	public:
		std::list<std::string> URIParts;
		std::string body;
		std::map<std::string, std::string> requestData;
		R_Authorization authorization;
		std::string tag;
		HandlerMethod method;
		R_SessionVersion version;
		HandlerRequest()
		{
			authorization = RA_NULL;
			version = RSV_NULL;
			method = HM_NULL;
		}
		HandlerRequest(const HandlerRequest &) = delete;
		HandlerRequest & operator=(const HandlerRequest &) = delete;
		HandlerRequest(HandlerRequest && other)
		: URIParts(std::move(other.URIParts)), 
			body(std::move(other.body)),
			requestData(std::move(other.requestData)),
			tag(std::move(other.tag))
		{}
	};
	class HandlerResponseMeta {
	public:
		std::map<std::string, std::string> headers;
		R_ResponseCode code;
		std::string tag;
		R_ResponseType type;
		HandlerResponseMeta() 
		: type(RRT_STRING), code(RSC_ERR_TEAPOT)
		{}
		HandlerResponseMeta & operator=(const HandlerResponseMeta &) = delete;
		HandlerResponseMeta(const HandlerResponseMeta &) = delete;
		HandlerResponseMeta(HandlerResponseMeta && other)
		: headers(std::move(other.headers)), code(other.code),
		type(other.type)
		{}
		HandlerResponseMeta & operator=(HandlerResponseMeta && other)
		{
			headers = std::move(other.headers);
			code = other.code;
			tag = std::move(other.tag);
			type = other.type;
			return *this;
		}
	};
	class HandlerResponse {
	public:
		std::string body;
		HandlerResponseMeta meta;
		HandlerResponse() = default;
		HandlerResponse & operator=(const HandlerResponse &) = delete;
		HandlerResponse(const HandlerResponse &) = delete;
		HandlerResponse(HandlerResponse && other)
		: body(std::move(other.body)), meta(std::move(other.meta))
		{}
		HandlerResponse & operator=(HandlerResponse && other)
		{
			body = std::move(other.body);
			meta = std::move(other.meta);
			return *this;
		}
	};
	class Handler {
	private:
		std::list<Handler*> hard_handles;
		std::list<Handler*> regex_handles;
	public:
		virtual ~Handler() {}
		typedef std::list<std::string>::iterator Lit;
		typedef HandlerResponse Response;
		typedef HandlerRequest Request;
		virtual const char * handle() = 0;
		virtual const bool isRegex() = 0;
		virtual void match(const std::string & part, Request &) = 0;
		virtual Response get(Request &) = 0;
		virtual Response put(Request &) = 0;
		virtual Response post(Request &) = 0;
		virtual Response del(Request &) = 0;
		virtual Response options(R_Authorization) = 0;
		virtual R_Authorization authorize(const std::string &, R_SessionVersion) = 0;
		Handler & resolve(const std::string & Part, Request * = nullptr);
		void addHandler(Handler *);
		//static Handler & getNullHandler();
	};
	//Still an abstract class, handle and isRegex is not
	//implemented.
	class BaseHandler : public Handler {
	public:
		virtual Response get(Request &);
		virtual Response put(Request &);
		virtual Response post(Request &);
		virtual Response del(Request &);
		virtual Response options(R_Authorization);
		virtual void match(const std::string &, Request &);
	};
};