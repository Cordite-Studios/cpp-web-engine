#pragma once
#include "utility/worker.hpp"

namespace Cordite {
	class WorkerGroup {
		Cordite::Worker w;
		class WorkerGroupInternals * _;
	public:
		typedef double LoadAvg;
		WorkerGroup(LoadAvg min, LoadAvg max);
		void work(); //! Work Locally
		void work(const char *); //! Work on a remote server
		~WorkerGroup();
	};	
};