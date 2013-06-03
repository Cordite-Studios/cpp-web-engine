#pragma once

namespace Cordite {
	class WorkerGroup;
	class Dispatcher {
	public:
		static const char * backend;
		typedef unsigned short Port;
		static void start(Port, WorkerGroup &);
		static void start(Port);
		static void shutdown();
	};
}