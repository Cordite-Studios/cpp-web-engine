#include "utility/memory/keeper.hpp"
#include "utility/logging/logger.hpp"
#include "utility/networking/zeromq.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <iomanip>
using namespace Cordite::Memory;
using namespace Cordite::Logging;
using namespace Cordite::Networking;
int main()
{
	Logger::start();
	ThreadKeeper::start();
	std::this_thread::sleep_for(std::chrono::nanoseconds(4000));
	ThreadKeeper::addReference("Apples", (void*)0xDEADBEEF);
	ThreadKeeper::delReference("Apples", (void*)0xDEADBEEF);
	
	ThreadKeeper::addReference("BANANAS", (void*)0xDEAD);
	ThreadKeeper::addReference("Meatless", (void*)0xBEEF);
	Logger::log(LGT_SILLY, "I am a funny guy");
	Logger::output(LGO_CERR, LGT_SILLY);
	Logger::log(LGT_SILLY, "I am a funny guy2");
	Logger::output(LGO_CERR, LGT_WARN);
	Logger::output(LGO_FILE, LGT_WARN, "somefile.log");
	std::this_thread::sleep_for(std::chrono::nanoseconds(4000));
	Logger::log(LGT_SILLY, "I might or might not show up");
	Logger::log(LGT_WARN, "But I should show up!");
	Logger::log(LGT_ERROR, "AND some ;-; ILLEGAL ;_; characters!");
	ThreadKeeper::addReference("Apples", (void*)0xDEADEEEEEE);
	ThreadKeeper::end();//This should happen before the logger goes
	Logger::shutdown();
	ZeroMQ::closeContext();
	std::cout << "And the program is now done." << std::endl;
}