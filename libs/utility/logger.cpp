#include <thread>
#include <sstream>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <set>
#include <map>
#include <fstream>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <regex>
#include "utility/networking/zeromq.hpp"
#include "utility/logging/logger.hpp"

namespace Cordite {
	namespace Logging {
//----------------------------
using namespace Networking;
std::atomic<bool> loggerActive(false);
std::mutex loggerCalled;
std::atomic<unsigned long> outputDest(LGO_CERR);
std::atomic<unsigned long> threadsUp;
std::condition_variable shutdownComplete;
std::thread * logger_thread_ptr = nullptr;
const char * logServerThread = "inproc://sys/logger";
const char * cerrLimitTag = "std::cerr";
const char * logDelimiter = ";-;";
const char * logEnd = ";_;";
const std::string escapeReplace("%-%");
std::atomic<LogType> lowestLog(LGT_WARN);

static __thread zmq::socket_t * sock = nullptr;
static __thread int active = 0;
static __thread std::string thread_name;

inline void logTime(std::ostream & o)
{
		using std::chrono::duration_cast;
		using std::chrono::milliseconds;
		using std::chrono::microseconds;
		using std::chrono::seconds;
		using std::chrono::system_clock;
		auto now = system_clock::now();
		auto secs = duration_cast<seconds>(now.time_since_epoch());
		auto frac = duration_cast<microseconds>(now.time_since_epoch() - secs).count();
		auto now_t = system_clock::to_time_t(now);
		o << std::put_time(std::gmtime(&now_t), "%FT%T.");
		o << std::setfill ('0') << std::setw (6) << frac;
}
inline void sanatize(std::ostream & o, std::string msg)
{
	const std::regex esc(";-;|;_;");
	bool needsSanatize = false;
	if(msg.find(logDelimiter) != std::string::npos)
	{
		needsSanatize = true;
	}
	if(msg.find(logEnd) != std::string::npos)
	{
		needsSanatize = true;
	}
	if(needsSanatize)
	{
		o << std::regex_replace(msg, esc, escapeReplace);
	}
	else
	{
		o << msg;
	}
}
inline const char * getTypeName(LogType t)
{
	switch(t)
	{
		case LGT_NULL: return "NULL";
		case LGT_INSANE: return "INSANE";
		case LGT_SILLY: return "SILLY";
		case LGT_VERBOSE: return "VERBOSE";
		case LGT_INFO: return "INFO";
		case LGT_NOTICE: return "NOTICE";
		case LGT_WARN: return "WARNING";
		case LGT_MEGAWARN: return "BIG WARNING";
		case LGT_ERROR: return "ERROR";
		case LGT_FATAL: return "FATAL";
		case LGT_NO_USE: return "NOT USED";
		default: return "????????????";
	}
}
static std::mutex loggerInstance;
void logger_thread()
{
	std::unique_lock<std::mutex> lock(loggerInstance);
	try
	{
		auto & context = ZeroMQ::getContext();
		zmq::socket_t receiver (context, ZMQ_PULL);
		receiver.bind(logServerThread);
		loggerActive = true;
		std::map<std::string, zmq::socket_t *> outputs;
		std::map<std::string, std::fstream *> outputFiles;
		std::map<std::string, std::function<void(std::list<std::string> &)>> actions;
		std::map<std::string, LogType> limits;
		limits[cerrLimitTag] = LGT_WARN;

		bool endOfLife = false;
		auto yeildLimit = [&](std::list<std::string> & list) -> const LogType {
			std::stringstream s;
			s << list.front();
			list.pop_front();
			int lim;
			s >> lim;
			return (LogType)lim;
		};
		auto processLimit = [&](const std::string & name, std::list<std::string> & list) {
			auto l = yeildLimit(list);
			if(name.size() == 0) return;
			limits[name] = l;
			if(l < lowestLog)
			{
				lowestLog = l;
			}
			else
			{
				//We possibly need to update.
				LogType lowest = LGT_ERROR;//highest
				for(auto limit : limits)
				{
					if(limit.second < lowest)
					{
						lowest = limit.second;
					}
				}
				lowestLog = lowest;
			}
		};

		actions["cerr"] = [&](std::list<std::string> & list){
			processLimit(cerrLimitTag, list);
		};
		actions["addServer"] = [&](std::list<std::string> & list){

			zmq::socket_t * socket = new zmq::socket_t(context, ZMQ_PUSH);
			auto server = list.front();
			list.pop_front();
			processLimit(server, list);
			socket->connect(server.c_str());
			outputs[server] = socket;
			//TODO: Add to memory keeper
		};
		actions["addFile"] = [&](std::list<std::string> & list){
			auto file = list.front();
			list.pop_front();
			auto fs = new std::fstream(file.c_str(), std::fstream::out | std::fstream::app);
			if(fs->good())
			{
				outputFiles[file] = fs;
				processLimit(file, list);
			}
			else
			{
				fs->close();
				processLimit("", list);
				//We can't really tell anyone this file couldn't
				//open correctly.
			}
		};
		actions["delServer"] = [&](std::list<std::string> & list){
			auto server = list.front();
			auto it = outputs.find(server);
			if(it != outputs.end())
			{
				delete it->second;
				outputs.erase(it);
			}
			//TODO: Add to memory keeper
		};
		actions["delFile"] = [&](std::list<std::string> & list){
			auto file = list.front();
			auto it = outputFiles.find(file);
			if(it != outputFiles.end())
			{
				delete it->second;
				outputFiles.erase(it);
			}
		};
		/*
			Message format:
			* Time
			* Log Type
			* Thread ID
			* Message
		*/
		actions["log"] = [&](std::list<std::string> & list){
			using std::chrono::system_clock;
			std::stringstream fullMsg;
			//Get the time
			fullMsg << list.front() << logDelimiter; list.pop_front();
			//Got the time

			
			//Get the type
			std::stringstream ts;
			ts << list.front();
			list.pop_front();
			int ti;
			ts >> ti;
			LogType type = (LogType)ti;
			fullMsg << std::setfill (' ') << std::setw (12);
			fullMsg << getTypeName(type) << logDelimiter;
			//Got the type

			//Get the thread ID
			fullMsg << list.front() << logDelimiter; list.pop_front();
			//Got the thread ID

			//Get the message
			auto msg = list.front(); list.pop_front();
			sanatize(fullMsg, msg);
			fullMsg << logEnd;
			//Got the message

			//Put the message
			auto fullStr = fullMsg.str();
			if(outputDest & LGO_CERR && type >= limits[cerrLimitTag])
			{
				std::cerr.flush();
				std::cerr << fullStr << std::endl;
				std::cerr.flush();
			}
			for(auto socket : outputs)
			{
				if(type < limits[socket.first]) continue;
				ZeroMQ::send(*socket.second,fullStr);
			}
			for(auto fs : outputFiles)
			{
				if(type < limits[fs.first]) continue;
				if(!fs.second->good()) continue;
				(*fs.second) << fullStr << std::endl;
			}
		};
		actions["end"] = [&](std::list<std::string> &){
			endOfLife = true;
		};
		
		while(true)
		{
			std::list<std::string> msg;
			std::string type;
			try
			{
				msg = ZeroMQ::receive(receiver);
				if(msg.empty()) continue;
				type = msg.front();
				if(type == "") continue;
				msg.pop_front();
				actions[type](msg);
				if(endOfLife)
					break;
			}
			catch (...)
			{
				//we only get an empty type if we are ending the application.
				if(type != "")
				{
					std::cerr << "System Logger had a fatal error..?";
					std::cerr << " Input was " << type;
					std::cerr << std::endl;
					std::cerr.flush();
					//std::terminate();
				}
			}
		}
		for(auto socket : outputs)
		{
			delete socket.second;
		}
		for(auto file : outputFiles)
		{
			try
			{
				file.second->close();
				delete file.second;
			}
			catch(...)
			{
				//We don't care.
			}
		}
		loggerActive = false;
		shutdownComplete.notify_all();
	}
	catch(zmq::error_t & ex)
	{
		std::cerr << "Address likely already in use?" << std::endl;
		std::cerr.flush();
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		loggerActive = true;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
		loggerActive = false;
	}
	catch(std::exception & ex)
	{
		std::cerr << "Got an exception during the entire log thread.." << std::endl;
		std::cerr << ex.what() << std::endl;
		std::terminate();
	}
	catch(...)
	{
		std::cerr << "Bad things happened in the logger, dunno what" << std::endl;
		std::terminate();
	}
}


void Logger::start()
{
	active++;
	if(sock) return;
	if(!loggerActive)
	{
		std::unique_lock<std::mutex> guard(loggerCalled);
		if(!loggerActive)
		{
			//std::thread logging(logger_thread);
			//logging.detach();
			logger_thread_ptr = new std::thread(logger_thread);
			//Apparently, unlike any other ZMQ socket
			//inproc:// sockets require that the other
			//already be connected.
			while(!loggerActive)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		}
	}
	sock = new zmq::socket_t(ZeroMQ::getContext(), ZMQ_PUSH);
	int milliTimeout = 35;
	sock->setsockopt(ZMQ_SNDTIMEO, &milliTimeout, sizeof(milliTimeout));
	try
	{
		sock->connect(logServerThread);
	} catch (zmq::error_t & t) {
		std::cerr << t.what() << std::endl;
		delete sock;
		throw;
	}
	threadsUp++;

}
void Logger::shutdown()
{
	if(!sock) return;
	if(--active != 0) return;
	//TODO: Shutdown
	if(--threadsUp == 0)
	{
		std::unique_lock<std::mutex> guard(loggerCalled);
		ZeroMQ::send(*sock, "end");
		while(loggerActive)
		{
			shutdownComplete.wait_for(guard, std::chrono::milliseconds(50));
		}
		if(logger_thread_ptr)
		{
			if(logger_thread_ptr->joinable())
			{
				logger_thread_ptr->join();
			}
			delete logger_thread_ptr;
			logger_thread_ptr = nullptr;
		}
	}
	if(sock)
	{
		delete sock;
		sock = nullptr;
	}
}
void Logger::log(LogType type, const char * const text)
{
	if(type < lowestLog) return;
	if(!sock)
	{
		std::cout << "PROBLEM: No socket associated with this thread! But to help.. " << std::endl;
		std::cout << "MESSAGE: " << text << std::endl;
		std::cout.flush();
		return;
	}

	std::stringstream types, id, thetime;
	types << (int)type;
	id << std::setw(12);
	if(thread_name.size() == 0)
	{
		id << "0x" << std::hex << std::this_thread::get_id();
	}
	else
	{
		id << thread_name;
	}
	logTime(thetime);
	try
	{
		ZeroMQ::send(*sock, "log", true);
		ZeroMQ::send(*sock, thetime.str(), true);
		ZeroMQ::send(*sock, types.str(), true);
		ZeroMQ::send(*sock, id.str(), true);
		ZeroMQ::send(*sock, text);
	}
	catch(zmq::error_t & ex)
	{
		std::cout << "PROBLEM: Sending log to manager failed! " << std::endl;
		std::cout << "MESSAGE: " << ex.what() << std::endl;
		std::cout.flush();
		return;
	}
}
void Logger::log(LogType type, std::function<void(std::ostream &)> fun)
{
	if(type < lowestLog) return;
	std::stringstream str;
	fun(str);
	log(type,str.str().c_str());
}

void Logger::output(const LogOutput output, const LogType type, const char * const serv_file)
{
	if(!sock) return;
	if(type < lowestLog)
	{
		lowestLog = type;
	}
	outputDest |= output;
	std::stringstream s;
	s << (int)type;
	switch(output)
	{
		case LGO_CERR:
		{
			ZeroMQ::send(*sock, "cerr", true);
			ZeroMQ::send(*sock, s.str());
		}break;
		case LGO_NETWORK:
		{
			ZeroMQ::send(*sock, "addServer", true);
			ZeroMQ::send(*sock, serv_file, true);
			ZeroMQ::send(*sock, s.str());
		} break;
		case LGO_FILE:
		{
			ZeroMQ::send(*sock, "addFile", true);
			ZeroMQ::send(*sock, serv_file, true);
			ZeroMQ::send(*sock, s.str());
		} break;
		default: break;
	}
}
void Logger::remOutput(const LogOutput output, const char * const serv_file)
{
	if(!sock) return;
	switch(output)
	{
		case LGO_CERR:
		{
			outputDest &= ~(unsigned long)(output);
		} break;
		case LGO_NETWORK:
		{
			ZeroMQ::send(*sock, "delServer", true);
			ZeroMQ::send(*sock, serv_file);
		} break;
		case LGO_FILE:
		{
			ZeroMQ::send(*sock, "delFile", true);
			ZeroMQ::send(*sock, serv_file);
		} break;
		default: break;
	}
}

void Logger::setName(const char * const name)
{
	thread_name = name;
}

//----------------------------
	};
};