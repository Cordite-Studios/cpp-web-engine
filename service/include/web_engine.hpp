#pragma once
#include <string>
#include <list>
#include "utility/worker.hpp"
#include "handler.hpp"
namespace Cordite {
	class WebEngine {
	public:
		static std::list<std::string> evaluate(std::list<std::string> & msg);
		static HandlerResponse evaluate(HandlerRequest & req);
	};	
};