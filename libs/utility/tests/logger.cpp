#include <thread>
#include <chrono>
#include "utility/logging/logger.hpp"

using std::thread;
using namespace Cordite::Logging;
void run1()
{
	Logger::start();
	std::chrono::milliseconds sh(431);
	for(int i = 0; i < 10; i++)
	{
		Logger::output(LGO_CERR, LGT_SILLY);
		std::this_thread::sleep_for(sh);
		Logger::output(LGO_CERR, LGT_WARN);
		std::this_thread::sleep_for(sh * 2);
	}
	Logger::shutdown();
}
void run2(unsigned int len)
{
	Logger::start();
	std::chrono::milliseconds sh(len);
	for(int i = 0; i < 40; i++)
	{
		Logger::log(LGT_SILLY, "I am a funny guy");
		Logger::log(LGT_WARN, "But I should show up!");
		std::this_thread::sleep_for(sh);
	}
	Logger::shutdown();
}

int main()
{
	Logger::start();
	thread t1(run1);
	thread t2(run2, 413);
	thread t3(run2, 643);
	thread t4(run2, 87);
	thread t5(run2, 809);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	Logger::shutdown();
}