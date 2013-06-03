#include <iostream>
#include "utility/memory/keeper.hpp"
#include "utility/logging/logger.hpp"
#include "handler.hpp"
#include "handler_registrar.hpp"
#include "dispatcher.hpp"
#include "worker_group.hpp"

using namespace Cordite::Memory;
using namespace Cordite::Logging;
using namespace Cordite;
int main()
{
	ScopedLogger log;
	ScopedKeeper k;
	HandlerRegistrar::init();
	Logger::setName("Test Giver");
	Logger::output(LGO_CERR, LGT_VERBOSE);
	WorkerGroup wg(0,80);
	Dispatcher::start(13456, wg);
	while(true)
	{
		std::this_thread::sleep_for(std::chrono::seconds(4));
	}
	Dispatcher::shutdown();
}