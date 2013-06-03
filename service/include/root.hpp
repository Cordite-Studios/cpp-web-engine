#include "handler.hpp"

namespace Cordite {
	class RootHandler : public BaseHandler
	{
	public:
		virtual const char * handle();
		virtual const bool isRegex();
		RootHandler() = default;
		virtual R_Authorization authorize(const std::string &, R_SessionVersion);
	};	
};