#include <thread>
#include <atomic>
#include <exception>
#include <sstream>
#include <condition_variable>
#include "utility/worker.hpp"
#include "utility/memory/keeper.hpp"
#include "utility/logging/logger.hpp"
#include "zmq.hpp"
using namespace Cordite::Memory;
using namespace Cordite::Logging;
namespace Cordite {
//----------------------------------
static std::atomic<unsigned int> worker_new_id(1);
class WorkerScopedBool {
	bool & scope;
public:
	WorkerScopedBool(bool & s) : scope(s)
	{
		scope = true;
	}
	~WorkerScopedBool()
	{
		scope = false;
	}
};
class WorkerInternals {
public:
	unsigned int id;
	std::thread t;
	Worker::Helper h;
	bool working;
	std::condition_variable started, ready, ending;
	std::mutex startHelper, readyHelper;
	bool has_started;
	WorkerInternals()
	{
		working = false;
		id = worker_new_id++;
		has_started = false;
	}
	WorkerInternals(const Worker &) = delete;
	Worker & operator=(const Worker &) = delete;
	static void worker(WorkerInternals * i)
	{
		ScopedKeeper k;
		ScopedLogger l;
		std::stringstream workerid;
		workerid << "Worker:" << i->id;
		Logger::setName(workerid.str().c_str());
		WorkerScopedBool t(i->has_started);
		i->started.notify_all();
		try {
			Logger::log(LGT_VERBOSE, "Worker Thread starting.");
			WorkerScopedBool(i->working);
			if(i->h.work) i->h.work(i->h);
			if(i->h.work2) i->h.work2(i->h, i->ready);
			Logger::log(LGT_VERBOSE, "Worker Thread shutting down.");
			if(i->h.onShutdown) i->h.onShutdown(i->h);
			i->ending.notify_all();
		}
		//Can't throw it, no one will get it.
		catch(zmq::error_t & ex)
		{
			Logger::log(LGT_ERROR,[&ex](std::ostream & o) {
				o << "Thread terminated unexpectedly! ";
				o << "ZeroMQ Exception: ";
				o << ex.what();
			});
		}
		catch(std::exception & ex)
		{
			Logger::log(LGT_ERROR,[&ex](std::ostream & o) {
				o << "Thread terminated unexpectedly due to: ";
				o << ex.what();
			});
		}
		catch(...)
		{
			Logger::log(LGT_ERROR, 
				"Thread terminated unexpectedly for unknown error!"
				);
		}
		i->working = false;
		i->ending.notify_all();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	~WorkerInternals()
	{
		if(t.joinable())
		{
			Logger::log(LGT_ERROR,
						"Thread should already be dead!"
						);
		}
	}
};
Worker::Worker()
: _(new WorkerInternals())
{

}
Worker::~Worker()
{
	if(!workDone())
	{
		stopWork();
	}
	if(_) delete _;
}
void Worker::setWork(Helper h)
{
	_->h = h;
}
void Worker::doWork()
{
	_->t = std::thread(WorkerInternals::worker, _);
	while(!_->has_started)
	{
		std::unique_lock<std::mutex> lk(_->startHelper);
		_->started.wait_for(lk, std::chrono::milliseconds(15));
	}
}
bool Worker::ensureReady()
{
	if(!_->h.supportsReady) return false;
	if(_->h.isReady) return true;
	std::unique_lock<std::mutex> lk(_->readyHelper);
	return _->ready.wait_for(lk, std::chrono::seconds(1), [&](){
		return _->h.isReady;
	});
}
void Worker::stopWork()
{
	if(workDone()) return;
	//if(_->h.requestedShutdown) return;
	//runs in this thread
	Logger::log(LGT_INSANE, "Calling stop work");
	if(_->h.doShutdown) _->h.doShutdown(_->h);
	_->h.requestedShutdown = true;
	while(_->working)
	{
		std::unique_lock<std::mutex> lk(_->startHelper);
		_->ending.wait_for(lk, std::chrono::milliseconds(15));
	}
	if(_->t.joinable())
	{
		_->t.join();
	}
	while(_->t.joinable())
	{
		Logger::log(LGT_SILLY, "Gah! Spurious Wakeup!");
		_->t.join();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
void Worker::stopWorkAsync()
{
	if(workDone()) return;
	if(_->h.requestedShutdown) return;
	//runs in this thread
	if(_->h.doShutdown) _->h.doShutdown(_->h);
	_->h.requestedShutdown = true;
}
bool Worker::workDone() const {
	if(!_) return true;
	return !_->working && !_->t.joinable();
}
Worker::Helper::Helper()
{
	supportsReady = false;
	context = nullptr;
	isReady = false;
	requestedShutdown = false;
}
void Worker::Helper::setWork(Helper::HelperFunc w)
{
	work = w;
}
void Worker::Helper::setWork(Helper::HelperFunc2 w)
{
	supportsReady = true;
	work2 = w;
}
unsigned int Worker::workerID() const {
	return _->id;
}
void * Worker::getContext() const {
	return _->h.context;
}
//----------------------------------	
};