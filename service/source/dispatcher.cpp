#include <sstream>
#include <map>
#include <queue>
#include "dispatcher.hpp"
#include "utility/worker.hpp"
#include "worker_group.hpp"
#include "zmq.hpp"
#include "utility/networking/zeromq.hpp"
#include "utility/logging/logger.hpp"
using namespace Cordite;
using namespace Cordite::Networking;
using namespace Cordite::Logging;
using std::string;
using std::stringstream;
namespace Cordite {
//---------------------------
static Worker dispatch_worker;
const char * Dispatcher::backend = "inproc://DispatcherBackend";
const int worker_poll = 300;
const int live_default = (1000.0/(double)worker_poll)*4;
struct DispatchWorker {
	string identity;
	int liveliness;
	bool isInner;
	bool pendingHeartbeat;
};
class DispatcherContext {
private:
	zmq::socket_t * front;
	zmq::socket_t * back;
	zmq::socket_t * backInner;
	std::map<string, DispatchWorker *> workers;
	std::queue<string> ready;
	std::list<string> accidentalMessage;
	bool poll_front;
	std::list<string> msg;
public:
	DispatcherContext(Dispatcher::Port port)
	{
		poll_front = false;
		stringstream s;
		s << "tcp://*:" << port;
		auto & context = ZeroMQ::getContext();
		front = new zmq::socket_t(context, ZMQ_ROUTER);
		int milliTimeout = 1;
		front->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
		front->bind(s.str().c_str());

		s.clear();//clear any bits set
		s.str(string());
		s << "tcp://*:" << (port + 1);
		back = new zmq::socket_t(context, ZMQ_ROUTER);
		back->bind(s.str().c_str());
		backInner = new zmq::socket_t(context, ZMQ_ROUTER);
		back->bind(Dispatcher::backend);
		back->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
		backInner->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
	}
	~DispatcherContext()
	{
		delete front;
		delete back;
		delete backInner;
	}
	bool shutdown(bool req)
	{
		if(!req) return false;
		for(auto workerPair : workers)
		{
			delete workerPair.second;
		}
		workers.clear();
		return req;
	}
	void processMessage(bool isInner = false)
	{
		string identity = msg.front();
		msg.pop_front();
		msg.pop_front();//Pop off the null entry.
		if(msg.size() == 1)
		{
			if(msg.front() == "READY")
			{
				Logger::log(LGT_INSANE, "Got a ready request");
				auto it = workers.find(identity);
				if(it != workers.end())
				{
					//This worker is in the incorrect state.
					//Let it die and come back later..
					return;
				}
				//This worker is new!
				//This is where hand shaking might happen
				//But for now, we just go with 
				// "Hello, get right on in!"
				auto worker = new DispatchWorker();
				worker->liveliness = live_default;
				worker->identity = identity;
				worker->isInner = isInner;
				worker->pendingHeartbeat = false;
				workers.insert(std::make_pair(identity, worker));
				ready.push(identity);
				poll_front = true;
			}
			else if(msg.front() == "DEATH")
			{
				Logger::log(LGT_INSANE, "Got a death notice");
				auto it = workers.find(identity);
				if(it != workers.end())
				{
					delete it->second;
					workers.erase(it);
					//The queue checks if it exists
					//there before it sends anywhere.
					//So, we may get an accidental
					//message, but it is easier
					//to just let the queue empty itself
					//than to dequeue and requeue in case..
				}
			}
			else if(msg.front() == "PONG")
			{
				Logger::log(LGT_INSANE, "Got a ready heart beat");
				auto it = workers.find(identity);
				if(it == workers.end())
				{
					//This worker took too long.
					//It is in the wrong state.
					//Let it die and come back later.
					//If we do more complex hand shaking
					//It may not have completed it and be
					//a bad one.
					return;
				}
				it->second->liveliness = live_default;
				it->second->pendingHeartbeat = false;
			}
		}
		else
		{
			Logger::log(LGT_INSANE, "Got a response to send");
			//Do actual requests!
			ZeroMQ::send(*front, msg);
			auto it = workers.find(identity);
			if(it == workers.end())
			{
				//This worker is no longer in our pool
				//and is not considered alive. It may
				//be a false connection too. So let's
				//Not give it more work.
				return;
			}
			//Put it back on the queue.
			ready.push(identity);
			poll_front = true;
		}
	}
	bool sendRequest()
	{
		if(msg.size() == 0) return false;

		DispatchWorker * worker = nullptr;
		while(!worker)
		{
			if(ready.empty())
			{
				poll_front = false;
				accidentalMessage = msg;
				return false;
			}
			string identity = ready.front();
			ready.pop();
			if(ready.empty())
			{
				//We are out of workers to handle requests.
				poll_front = false;
			}
			//Check to see if this worker is not stale
			auto it = workers.find(identity);
			if(it == workers.end())
			{
				//it is indeed stale..
				worker = nullptr;
			}
			else
			{
				worker = it->second;
			}
		}
		
		//now for the real business.
		if(worker->isInner)
		{
			ZeroMQ::send(*backInner, worker->identity, true);
			//Learned this the hard way
			ZeroMQ::send(*backInner, "", true);
			ZeroMQ::send(*backInner, msg);
		}
		else
		{
			ZeroMQ::send(*back, worker->identity, true);
			//Learned this the hard way
			ZeroMQ::send(*back, "", true);
			ZeroMQ::send(*back, msg);
		}
		return true;
	}
	void doRegularProcess()
	{
		std::queue<string> toDelete;
		int heartsSent = 0;
		for(auto workerPair : workers)
		{
			auto worker = workerPair.second;
			if(!worker->pendingHeartbeat)
			{
				heartsSent++;
				if(worker->isInner)
				{
					ZeroMQ::send(*backInner, worker->identity, true);
					//Learned this the hard way
					ZeroMQ::send(*backInner, "", true);
					ZeroMQ::send(*backInner, "PING");
				}
				else
				{
					ZeroMQ::send(*back, worker->identity, true);
					//Learned this the hard way
					ZeroMQ::send(*back, "", true);
					ZeroMQ::send(*back, "PING");
				}
				worker->pendingHeartbeat = true;
			}
			worker->liveliness--;
			if(worker->liveliness <= 0)
			{
				toDelete.push(worker->identity);
			}
		}
		if(heartsSent)
		{
			Logger::log(LGT_INSANE, "Sent Heartbeats");
		}
		while(!toDelete.empty())
		{
			Logger::log(LGT_INSANE, "Cleaning up some workers.");
			auto it = workers.find(toDelete.front());
			toDelete.pop();
			if(it != workers.end())
			{
				delete it->second;
				workers.erase(it);
			}
		}
	}
	void handleRequests(Worker::Helper & h)
	{
		zmq::pollitem_t items [] = {
			{ *backInner, 0, ZMQ_POLLIN, 0 },
			{ *back, 0, ZMQ_POLLIN, 0 },
			{ *front, 0, ZMQ_POLLIN, 0 },
		};
		
		string identity;
		
		auto start = std::chrono::steady_clock::now();
		auto end = start;
		while(true)
		{
			zmq::poll(items, poll_front ? 3 : 2, worker_poll);
			msg.clear();
			if(items[0].revents & ZMQ_POLLIN)
			{
				Logger::log(LGT_INSANE, "Got a response on backInner");
				msg = ZeroMQ::receive(*backInner);
				processMessage(true);
			}
			if(items[1].revents & ZMQ_POLLIN)
			{
				Logger::log(LGT_INSANE, "Got a response on back");
				msg = ZeroMQ::receive(*back);
				processMessage();
			}
			if(poll_front && ready.size() > 0 && items[2].revents & ZMQ_POLLIN)
			{
				Logger::log(LGT_INSANE, "Got a request.");
				if(accidentalMessage.size() > 0)
				{
					msg = std::move(accidentalMessage);
					if(sendRequest())
					{
						Logger::log(LGT_INSANE,
						"sent a response for accidental message");
						accidentalMessage.clear();
						//Process the normal message while
						//we are at it too.
						Logger::log(LGT_INSANE, "Handling side request");
						msg = ZeroMQ::receive(*front);
						sendRequest();
					}
					else
					{
						accidentalMessage = std::move(msg);
						Logger::log(LGT_INSANE, "Couldn't send accidental message");
					}
					
				}
				else
				{
					//Continue on with normal messages
					//If we had an accidentalMessage
					//and we just used the last worker
					//Then the following will create a
					//new accidental message too.
					Logger::log(LGT_INSANE, "Handling normal request");
					msg = ZeroMQ::receive(*front);
					sendRequest();
				}
				
			}
			end = std::chrono::steady_clock::now();
			if(end - start > std::chrono::milliseconds(worker_poll))
			{
				start = end;
				doRegularProcess();
			}
			if(shutdown(h.requestedShutdown)) break;
		}
		
	}
};


void Dispatcher_work(Worker::Helper & h)
{
	Logger::setName("Dispatcher");
	Logger::log(LGT_INFO, "Dispatcher starting.");
	auto _ = (DispatcherContext *)h.context;
	_->handleRequests(h);
}
void Dispatcher::start(Port port)
{
	Worker::Helper setup;
	//------
	DispatcherContext * context = new DispatcherContext(port);
	setup.context = context;
	setup.setWork(Dispatcher_work);
	setup.onShutdown = [](Worker::Helper & h) {
		delete (DispatcherContext*)h.context;
		h.context = nullptr;
	};
	//------
	dispatch_worker.stopWork();
	dispatch_worker.setWork(setup);
	dispatch_worker.doWork();
}
void Dispatcher::start(Port port, WorkerGroup & wg)
{
	Worker::Helper setup;
	//------
	DispatcherContext * context = new DispatcherContext(port);
	setup.context = context;
	setup.setWork(Dispatcher_work);
	setup.onShutdown = [](Worker::Helper & h) {
		delete (DispatcherContext*)h.context;
		h.context = nullptr;
	};
	//------
	dispatch_worker.stopWork();
	dispatch_worker.setWork(setup);
	dispatch_worker.doWork();
	wg.work();
}
void Dispatcher::shutdown()
{
	dispatch_worker.stopWork();
}


//---------------------------
};