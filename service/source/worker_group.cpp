#include <list>
#include <sstream>
#include <atomic>
#include <map>
#include <algorithm>
#include <random>
#include <exception>
#include <cstdlib>
#include <chrono>
#include "worker_group.hpp"
#include "utility/worker.hpp"
#include "utility/networking/zeromq.hpp"
#include "utility/logging/logger.hpp"
#include "dispatcher.hpp"
#include "web_engine.hpp"

using namespace Cordite;
using namespace Cordite::Networking;
using namespace Cordite::Logging;

namespace Cordite {
//-------------------------
static std::atomic<unsigned int> worker_group_counts(0);
const int worker_poll = 300;
const int live_default = (1000.0/(double)worker_poll)*4;
class WorkerContext {
public:
	zmq::socket_t * socket;
	zmq::socket_t * control;
	volatile bool requestedUpdate;
	volatile bool active;
	unsigned int id;
	int liveliness;
	std::list<std::string> route, msg;
	bool bad;
	WorkerContext()
	{
		socket = nullptr;
		control = nullptr;
		requestedUpdate = true;
		active = false;
		bad = false;
		liveliness = live_default;
	}
	~WorkerContext()
	{
		if(control) delete control;
		if(socket) delete socket;
	}
	void setup(const char * controller, const char * connectTo)
	{
		auto & context = ZeroMQ::getContext();
		socket = new zmq::socket_t(context, ZMQ_REQ);
		socket->connect(connectTo);
		int milliTimeout = 3500;
		socket->setsockopt(ZMQ_SNDTIMEO, &milliTimeout, sizeof(milliTimeout));
		control = new zmq::socket_t(context, ZMQ_REQ);
		control->connect(controller);
		milliTimeout = 100;
		control->setsockopt(ZMQ_SNDTIMEO, &milliTimeout, sizeof(milliTimeout));
		control->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
		control->setsockopt(ZMQ_LINGER, &milliTimeout, sizeof(milliTimeout));
		socket->setsockopt(ZMQ_LINGER, &milliTimeout, sizeof(milliTimeout));
		socket->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));
	}
	bool isHeartBeat()
	{
		if(route.size() == 1 && route.front() == "PING")
		{
			//It is a heart beat!
			//Just ask for more work.
			ZeroMQ::send(*socket, "PONG");
			Logging::Logger::log(Logging::LGT_SILLY, "Got a Heart beat");
			liveliness = live_default;
			return true;
		}
		return false;
	}
	void receive()
	{
		route.clear();
		msg.clear();
		route = ZeroMQ::receive(*socket);
	}
	void stripRoute()
	{
		auto it = route.end();
		//find the end of the encapsulation
		if(std::find(route.begin(), route.end(), "") != route.end())
		{
			while(*(--it) != "")
			{
				msg.push_front(*it);
			}
		}
		else
		{
			Logger::log(LGT_VERBOSE, "No envelope detected?");
			msg = route;
			route.erase(route.begin(), route.end());
		}
		it++;
		//clear the message portion out.
		route.erase(it, route.end());
	}
	void sendResponse()
	{
		auto resp = WebEngine::evaluate(msg);
		try
		{
			//Send back to the router where to return the message
			if(route.size() > 0)
			{
				ZeroMQ::send(*socket, route, true);
			}
			//Send the response
			ZeroMQ::send(*socket, resp);
		}
		catch(zmq::error_t & ex)
		{
			Logging::Logger::log(Logging::LGT_ERROR, [&ex](std::ostream & o){
				o << "Uncaught exception during sending? " << ex.what();
			});
		}
	}
	static void work(Worker::Helper & h, std::condition_variable & cond)
	{
		auto _ = (WorkerContext*)h.context;
		zmq::pollitem_t items [] = {
			{ *(_->socket), 0, ZMQ_POLLIN, 0 }
		};
		try {
			Logging::Logger::log(Logging::LGT_VERBOSE, "Sending Ready Request");
			ZeroMQ::send(*(_->socket), "READY");
			while(!h.requestedShutdown && _->liveliness-- > 0)
			{
				cond.notify_all();
				//Waiting for a response
				zmq::poll(items, 1, worker_poll);
				if(items[0].revents & ZMQ_POLLIN)
				{
					_->active = true;
					//We are alive!
					_->receive();
					
					if(_->isHeartBeat()) continue;
					//Maybe a request at this point.
					Logging::Logger::log(Logging::LGT_VERBOSE, "Got a request");
					_->stripRoute();
					
					_->sendResponse();
					Logging::Logger::log(Logging::LGT_VERBOSE, "Sent Reply");
					_->liveliness = live_default;
					_->active = false;
				}
			}
			if(h.requestedShutdown)
			{
				Logging::Logger::log(Logging::LGT_NOTICE, "Requested Death by parent");
			}
			Logging::Logger::log(Logging::LGT_INFO, "Worker terminating");
		}
		catch(std::exception & ex)
		{
			Logging::Logger::log(Logging::LGT_ERROR, [&ex](std::ostream & o){
				o << "Uncaught exception in a Cordite Worker "
				"while processing a request!" << ex.what();
			});
		}
		catch(...)
		{
			Logging::Logger::log(Logging::LGT_ERROR, 
				"Uncaught unknown Exception in A Cordite Worker "
				"while processing a request!"
				);
		}
		ZeroMQ::send(*(_->control), "DEAD", true);
		std::stringstream sid;
		sid << _->id;
		ZeroMQ::send(*(_->control), sid.str());
		ZeroMQ::receive(*(_->control));
	}
};

class WorkerGroupInternals {
public:
	const char * destination;
	WorkerGroup::LoadAvg min;
	WorkerGroup::LoadAvg max;
	std::map<unsigned int, Worker *> workers;
	std::map<unsigned int, WorkerContext *> contexts;
	zmq::socket_t * controlSocket;
	std::string internalSocketName;
	double loadAverages[3];
	std::default_random_engine generator;
	bool shuttingDown;

	WorkerGroupInternals()
	{
		min = 0;
		max = 100;
		
	}
	~WorkerGroupInternals()
	{
		Logger::log(LGT_SILLY, "Destructor for Worker Group Internals called");
	}
	Worker * ProcureWorker(bool & bad)
	{
		Worker * worker = new Worker();
		Worker::Helper h;
		h.setWork(WorkerContext::work);
		WorkerContext * c = new WorkerContext();
		try
		{
			c->setup(internalSocketName.c_str(), destination);
		}
		catch(zmq::error_t & ex)
		{
			c->bad = true;
			bad = true;
		}
		c->id = worker->workerID();
		h.context = c;
		h.onShutdown = [](Worker::Helper & h){
			delete (WorkerContext*)h.context;
		};
		worker->setWork(h);
		contexts[worker->workerID()] = c;
		if(!c->bad)
		{
			worker->doWork();
			worker->ensureReady();
		}
		else
		{
			h.context = nullptr;
			delete c;
		}
		return worker;
	}
	void addWorkers(unsigned int amount)
	{
		if(shuttingDown) return;
		Logging::Logger::log(Logging::LGT_NOTICE, "Creating workers");
		try
		{
			for(int i = 0; i < amount && !shuttingDown; i++)
			{
				bool bad = false;
				Worker * w = ProcureWorker(bad);
				if(bad || !w->getContext())
				{
					Logging::Logger::log(Logging::LGT_ERROR, "Worker setup failed.");
					delete w;
					break;
				}
				workers[w->workerID()] = w;
			}
		}
		catch(zmq::error_t & ex)
		{
			Logging::Logger::log(Logging::LGT_ERROR, [&ex](std::ostream & o){
				o << "Workers failed to start. " << ex.what();
			});
		}
	}
	void killRandomWorker()
	{
		if(workers.size() < 5) return;
		std::uniform_int_distribution<int> distribution(1, workers.size());
		int randomAdvance = distribution(generator);
		auto it = workers.begin();
		Logging::Logger::log(Logging::LGT_NOTICE, "Killing a Worker Thread");
		it->second->stopWorkAsync();
	}
	void receiveMessage()
	{
		//We have action!
		auto msg = ZeroMQ::receive(*controlSocket);
		if(msg.front() == "DEAD")
		{
			Logging::Logger::log(Logging::LGT_VERBOSE, "Got a worker death message");
			msg.pop_front();
			std::stringstream sid;
			sid << msg.front();
			msg.pop_front();
			unsigned int id = 0;
			sid >> id;
			ZeroMQ::send(*controlSocket, "OK");
			if(workers[id])
			{
				workers[id]->stopWork();
				delete workers[id];
				workers.erase(workers.find(id));
			}
		}
		else
		{
			ZeroMQ::send(*controlSocket, "?");
		}
	}
	bool shutdown(bool requested)
	{
		if(requested)
		{
			int up = 0;
			for(auto w : workers)
			{
				w.second->stopWorkAsync();
				up += w.second->workDone() ? 0 : 1;
			}
			if(up == 0)
			{
				Logging::Logger::log(Logging::LGT_NOTICE, "All Workers should be done now.");
				return true;
			}
			else
			{
				Logging::Logger::log(Logging::LGT_NOTICE, [up](std::ostream & o){
					o << "Worker Group Manager still running, ";
					o << up << " still running.";
				});;
			}
		}
		return false;
	}
	void doRegularProcess()
	{
		Logging::Logger::log(Logging::LGT_NOTICE, "Get some load averages!");
		int numres = getloadavg(loadAverages,3);
		if(numres <= 0) return;
		for(int i = 0; i < numres; i++)
		{
			loadAverages[i] /= (double)std::thread::hardware_concurrency();
			loadAverages[i] *= 100.0;
		}
		Logging::Logger::log(Logging::LGT_NOTICE, [&](std::ostream & o){
				o << "Amounts: " << loadAverages[0] << ", ";
				o << loadAverages[1] << ", " << loadAverages[2];
			});
		if(loadAverages[0] < min && workers.size() < 100)
		{
			//Don't make more than 100 threads, for now
			//TODO: Extract max worker count to be
			//configurable
			double partial = 1.0/(double)(workers.size());
			double amount = 0;
			for(auto pair : workers)
			{
				if(!pair.second) continue;
				if(contexts[pair.second->workerID()]->active)
					amount += partial;
			}
			Logging::Logger::log(Logging::LGT_NOTICE, [&](std::ostream & o){
				o << "Partial Count: " << amount;
			});
			if(partial > 0.85)
			{
				//85 % or more of our workers are used,
				//but we are underutilizing our CPU
				addWorkers(5);
			}
			else if(partial < 0.15)
			{
				Logging::Logger::log(Logging::LGT_NOTICE, "We are rather underutilized");
			}
			if(workers.size() < 2)
			{
				Logging::Logger::log(Logging::LGT_NOTICE, "Adding Workers, too few!");
				addWorkers(5);
			}
			
		}
		else if(loadAverages[0] > std::min(95.0, max))
		{
			killRandomWorker();
		}
	}
	static void doWork(Worker::Helper & h)
	{
		std::stringstream workerid;
		workerid << "WG Manager";
		Logger::setName(workerid.str().c_str());
		auto _ = (WorkerGroupInternals*)h.context;
		//Setup
		auto & context = ZeroMQ::getContext();
		_->controlSocket = new zmq::socket_t(context, ZMQ_REP);
		_->controlSocket->bind(_->internalSocketName.c_str());
		int milliTimeout = 10;
		_->controlSocket->setsockopt(ZMQ_SNDTIMEO, &milliTimeout, sizeof(milliTimeout));
		_->controlSocket->setsockopt(ZMQ_RCVTIMEO, &milliTimeout, sizeof(milliTimeout));

		//Begin main stuff
		zmq::pollitem_t items [] = {
			{ *(_->controlSocket), 0, ZMQ_POLLIN, 0 }
		};
		_->addWorkers(10);
		auto start = std::chrono::steady_clock::now();
		auto end = start;
		Logging::Logger::log(Logging::LGT_INFO, "Starting the manager loop");
		while(true)
		{
			zmq::poll (items, 1, 1000);
			if(items [0].revents & ZMQ_POLLIN)
			{
				_->receiveMessage();
			}
			end = std::chrono::steady_clock::now();
			if(end - start > std::chrono::milliseconds(6000))
			{
				start = end;
				_->doRegularProcess();
			}
			if(_->shutdown(h.requestedShutdown)) break;
			
		}
	}
	static void stop(Worker::Helper & h)
	{
		auto _ = (WorkerGroupInternals*)h.context;
		_->shuttingDown = true;
		Logger::log(Logging::LGT_SILLY, "Worker Group Terminating");
		if(!_->controlSocket)
		{
			Logger::log(LGT_NOTICE, "Control Socket missing..?");
			return;
		}
		delete _->controlSocket;
		_->controlSocket = nullptr;
	}
};
WorkerGroup::WorkerGroup(LoadAvg min, LoadAvg max)
{
	_ = new WorkerGroupInternals();
	_->min = min;
	_->max = max;
	_->shuttingDown = false;

	Worker::Helper h;
	h.context = _;
	h.setWork(WorkerGroupInternals::doWork);
	h.onShutdown = WorkerGroupInternals::stop;
	w.setWork(h);
	_->destination = nullptr;

	std::stringstream s;
	s << "inproc://workerGroup/" << worker_group_counts++;
	_->internalSocketName = s.str();
};
WorkerGroup::~WorkerGroup()
{
	w.stopWork();
	if(_) delete _;
	_ = nullptr;
}
void WorkerGroup::work()
{
	if(!_) return;
	//Work internally!
	_->destination = Dispatcher::backend;
	w.doWork();
}
void WorkerGroup::work(const char * external)
{
	if(!_) return;
	//Work externall, connect to the specified channel
	_->destination = external;
	w.doWork();
}
//-------------------------
};