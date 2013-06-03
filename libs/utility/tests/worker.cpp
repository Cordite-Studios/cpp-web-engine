#include <iostream>
#include <thread>
#include <algorithm>
#include <chrono>
#include "utility/worker.hpp"
#include "utility/logging/logger.hpp"
#include "utility/memory/keeper.hpp"

using namespace Cordite;
using namespace Cordite::Logging;
using namespace Cordite::Memory;

int main()
{
	Logger::start();
	Logger::output(LGO_CERR, LGT_INFO);
	Worker w;
	auto context = new char[40];
	const char happyContext[] = "Happy things";
	std::copy(happyContext, happyContext+sizeof(happyContext), context);
	Worker::Helper h;
	h.setWork([](Worker::Helper & h){
		char * c = (char*)h.context;
		ThreadKeeper::addReference("Worker Context", h.context);
		std::chrono::milliseconds sleepish(1000);
		for(int i = 0; i < 10; i++)
		{
			if(!c) break;
			if(h.requestedShutdown) break; 
			std::this_thread::sleep_for(sleepish);
			std::cout << "Did work. " << c << std::endl;
		}
		std::cout << "Work ending, but throwing exception" << std::endl;
		throw "I should be caught and logged";
	});
	//It is not necessary to have this if you use the
	//requestedShutdown var which is set on stopWork()
	/*h.doShutdown = [](Worker::Helper & h){
		std::cout << "commanding shutdown.." << std::endl;
	};*/
	h.onShutdown = [](Worker::Helper & h){
		ThreadKeeper::delReference("Worker Context", h.context);
		std::cout << "Shutting down" << std::endl;
		delete[] (char*)h.context;
	};
	h.context = context;
	w.setWork(h);
	w.doWork();
	std::this_thread::sleep_for(std::chrono::milliseconds(3000));
	w.stopWork();
	Logger::shutdown();
}