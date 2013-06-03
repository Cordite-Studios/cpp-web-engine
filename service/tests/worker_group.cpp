#include <thread>
#include <chrono>
#include <iostream>
#include "worker_group.hpp"
#include "utility/worker.hpp"
#include "utility/networking/zeromq.hpp"
#include "utility/logging/logger.hpp"

using namespace Cordite;
using namespace Networking;
using namespace Logging;
volatile bool started = false;
const int requests = 50;
void runStuff(Worker::Helper & h, std::condition_variable & cond)
{
	ScopedLogger l;
	Logger::setName("Test Giver");
	zmq::socket_t giver(ZeroMQ::getContext(), ZMQ_REP);
	giver.bind("inproc://test");
	int milliTimeout = 100;
	giver.setsockopt(ZMQ_LINGER, &milliTimeout, sizeof(milliTimeout));
	milliTimeout = 100;
	giver.setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
	started = true;
	for(int i = 0; i < requests; i++)
	{
		cond.notify_all();
		auto m = ZeroMQ::receive(giver);
		Logger::log(LGT_INFO, [&m](std::ostream & o){
			int c = 0;
			o << "Got a response: ";
			for(auto a : m)
			{
				o << a;
				if(a == "")
				{
					o << "{Null Frame}";
				}
				if(++c < m.size()) o << std::endl;
			}
			if(m.size() == 0)
			{
				o << "Well then. The reply was empty.";
			}
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(40));
		Logger::log(LGT_SILLY, "Sending a demo message");
		try {
			ZeroMQ::send(giver, "DEMOush");
		}
		catch(...)
		{
			//We must have timed out instead.
			//This is just a test, so it doesn't matter so much.
		}
	}
	try
	{
		ZeroMQ::receive(giver);
	}
	catch(...)
	{

	}
}
int main()
{
	ScopedLogger l;
	Logger::setName("Main");
	Logger::output(LGO_CERR, LGT_INFO);
	do
	{
		WorkerGroup wg(100,195);
		Worker w;
		Worker::Helper h;
		h.setWork(runStuff);
		w.setWork(h);
		w.doWork();
		w.ensureReady();
		wg.work("inproc://test");
		w.stopWork();
		Logger::log(LGT_SILLY, "Main tester shutting down.");
	}
	while(false);
	Logger::log(LGT_NOTICE, [](std::ostream & o){
		o << std::endl;
		o << "________________________________" << std::endl;
		o << "MAIN ending now!" << std::endl;
		o << "________________________________";
	});
	
	
	

}