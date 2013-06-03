#include <list>
#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include "dispatcher.hpp"
#include "handler_registrar.hpp"
#include "worker_group.hpp"
#include "utility/logging/logger.hpp"
#include "utility/networking/zeromq.hpp"

using namespace Cordite;
using namespace Cordite::Logging;
using namespace Cordite::Networking;
using std::list;
using std::string;
const int requests = 1000;
const int thread_count = 50;
void run()
{
	ScopedLogger l;
	zmq::socket_t giver(ZeroMQ::getContext(), ZMQ_REQ);
	giver.connect("tcp://localhost:13456");
	int milliTimeout = 1000;
	giver.setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));

	try
	{
		for(int i = 0; i < requests; i++)
		{
			list<string> req;
			req.push_back("Type:GET");
			req.push_back("URI:/demo");
			req.push_back("Tag:pickles");
			ZeroMQ::send(giver, req);
			auto response = ZeroMQ::receive(giver);
			Logger::log(LGT_INFO, [&response](std::ostream & o){
				o << "Got a response!" << std::endl;
				for(auto res : response)
				{
					o << res << std::endl;
				}
				o << "End of response.";
			});
		}
	}
	catch(zmq::error_t & ex)
	{
		const char * what = ex.what();
		std::cerr.flush();
		std::cerr << "ZeroMQ Exception in test: " << what;
		std::cerr << std::endl;
		std::cerr.flush();
	}
}
int main()
{
	ScopedLogger l;
	HandlerRegistrar::init();
	Logger::setName("Test Giver");
	Logger::output(LGO_CERR, LGT_NOTICE);
	WorkerGroup wg(0,80);
	Dispatcher::start(13456, wg);
	auto start = std::chrono::steady_clock::now(); 
	std::vector<std::thread> threads;
	for(int i = 0; i < thread_count; i++)
	{
		threads.push_back(std::thread(run));
	}
	for(auto & thread : threads)
	{
		thread.join();
	}
	auto end = std::chrono::steady_clock::now();
	Dispatcher::shutdown();
	Logger::log(LGT_NOTICE, [&](std::ostream & o){
		auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
		o << "Test of " << thread_count << " threads, a total of ";
		o << requests * thread_count << " requests took: ";
		o << mil.count();
		o << "ms." << std::endl;
		o << "Meaning, ";
		double reqs = (double)(requests * thread_count) / (double) mil.count();
		o << "We have a speed of " << reqs*1000.0 << " requests/s. ";
		o << "Or, in other words, each request took ";
		double req = 1.0/reqs;
		o << req << "ms.";
	});
	
}