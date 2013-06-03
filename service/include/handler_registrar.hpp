#pragma once
#include <map>
#include <string>

namespace Cordite {
	class Handler;
	class HandlerRegistrar {
		class HRInternals * _;
		HandlerRegistrar();
	public:
		~HandlerRegistrar();
		static HandlerRegistrar & get();
		void registerHandle(const std::string, Handler *);
		void registerTo(const std::string, Handler *);
		static void init();
		static Handler & getRoot();
	};
};