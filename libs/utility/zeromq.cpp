#include <mutex>
#include "utility/networking/zeromq.hpp"
namespace Cordite {
	namespace Networking {
//-------------------------
static std::mutex zmq_init_mutex;
static zmq::context_t * context = nullptr;
zmq::context_t & ZeroMQ::getContext()
{
	if(!context)
	{
		std::lock_guard<std::mutex> lock(zmq_init_mutex);
		if(!context)
		{
			context = new zmq::context_t(1);
		}
	}
	return *context;
}
void ZeroMQ::closeContext()
{
	if(context)
	{
		std::lock_guard<std::mutex> lock(zmq_init_mutex);
		if(context)
		{
			delete context;
			context = 0;
		}
	}
}


class ZeroMQWatcher {
public:
	ZeroMQWatcher()
	{

	}
	~ZeroMQWatcher()
	{
		//so main no longer needs to.
		ZeroMQ::closeContext();
	}
};
static ZeroMQWatcher zmq_watcher;

//-------------------------
	};
};