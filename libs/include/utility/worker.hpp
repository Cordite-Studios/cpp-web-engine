#pragma once
#include <functional>
#include <thread>


namespace Cordite {
	class Worker {
	private:
		class WorkerInternals * _;
	public:
		class Helper {
		public:
			friend class WorkerInternals;
			typedef std::function<void(Helper &)> HelperFunc;
			typedef std::function<void(Helper &, std::condition_variable &)> HelperFunc2;
		private:
			HelperFunc work;
			HelperFunc2 work2;
		public:
			HelperFunc doShutdown;
			HelperFunc onShutdown;
			volatile bool requestedShutdown;
			volatile bool isReady;
			bool supportsReady;
			void * context;
			Helper();
			void setWork(HelperFunc);
			void setWork(HelperFunc2);
		};
		Worker();
		Worker(const Worker &) = delete;//No copy!
		Worker & operator=(const Worker &) = delete;//No copy!
		virtual ~Worker();
		void setWork(Helper);
		void doWork();
		void stopWork();
		void stopWorkAsync();
		bool ensureReady();
		bool workDone() const;
		unsigned int workerID() const;
		void * getContext() const;
	};
};