#pragma once
#include <string>
#include <list>
#include <cstring>
#include <zmq.hpp>

namespace Cordite {
	namespace Networking {
		class ZeroMQ {
		public:
			static zmq::context_t & getContext();
			static void closeContext();
			static std::list<std::string> receive(zmq::socket_t &);
			static bool send(zmq::socket_t &, const std::string &, const bool = false);
			static bool send(zmq::socket_t &, const char * const, const bool = false);
			static bool send(zmq::socket_t &, const std::list<std::string> &, const bool = false);
		};

inline std::list<std::string> ZeroMQ::receive(zmq::socket_t & socket)
{
	std::list<std::string> ret;
	zmq::message_t message;
	do
	{
		try
		{
			socket.recv(&message);
			std::string part(static_cast<char*>(message.data()), message.size());
			ret.push_back(part);
		}
		catch(zmq::error_t & ex)
		{
			if(ex.num() == EINTR)
			{
				continue;
			}
			throw;
		}
		
	}
	while(message.has_more());

	return ret;
}
inline bool ZeroMQ::send(zmq::socket_t & socket, const std::string & content, const bool more)
{
	zmq::message_t message(content.size());
	memcpy (message.data(), content.data(), content.size());
	bool rc;
	if(more)
	{
		rc = socket.send (message, ZMQ_SNDMORE);
	}
	else
	{
		rc = socket.send (message);
	}
		
	return (rc);
}
inline bool ZeroMQ::send(zmq::socket_t & socket, const char * const content, const bool more)
{
	int len = strlen(content);
	zmq::message_t message(len);
	memcpy (message.data(), content, len);
	bool rc;
	if(more)
	{
		rc = socket.send (message, ZMQ_SNDMORE);
	}
	else
	{
		rc = socket.send (message);
	}
		
	return (rc);
}
inline bool ZeroMQ::send(zmq::socket_t & socket, const std::list<std::string> & content, bool more)
{
	bool good = true;
	int pos = 0;
	for(auto entry : content)
	{
		if(!good) return false;
		if(++pos == content.size()) break;
		good = good && send(socket, entry, true);
	}
	return send(socket, content.back(), more);

}


	};
};