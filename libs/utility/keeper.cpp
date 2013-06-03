#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include "utility/memory/keeper.hpp"
#include "utility/logging/logger.hpp"

using namespace std;

namespace Cordite {
	namespace Memory {
typedef unordered_set<const void *> MemEntry;
typedef unordered_map<string, MemEntry> MemMap;


class Keeper {
private:
	MemMap memEntries;
	unsigned long count;
public:
	Keeper()
	:count(0)
	{
		
	}
	~Keeper()
	{
		if(count)
		{
			//TODO: Have a logger listen to this instead. 
			Logging::Logger::log(Logging::LGT_MEGAWARN, [&](std::ostream & out){
				out << endl;
				out << "______________________________" << endl;
				out << "START LEAK DETECTED!" << endl;
				for(auto classEntry : memEntries)
				{
					if(classEntry.second.size())
					{
						out << classEntry.first<< " has " 
							<< classEntry.second.size() << " entries!" << endl;
					}
					for(auto instanceEntry : classEntry.second)
					{
						out << classEntry.first << " " << instanceEntry << endl;
					}
				}
				out << "END LEAK DETECTED!" << endl;
				out << "______________________________" << endl;
			});
		}
	}
	void push(const char * const name, const void * pointer)
	{
		memEntries[string(name)].insert(pointer);
		++count;
	}
	void pop(const char * const name, const void * pointer)
	{
		auto found = memEntries[string(name)].find(pointer);
		if(found != memEntries[string(name)].end())
		{
			memEntries[string(name)].erase(found);
			--count;
		}
	}
	const unsigned long getCount() const {
		return count;
	}
};

__thread Keeper * keeper = nullptr;

void ThreadKeeper::start()
{
	if(!keeper)
	{
		keeper = new Keeper();
	}
}
void ThreadKeeper::end()
{
	if(keeper)
	{
		delete keeper;
		keeper = nullptr;
	}
}

void ThreadKeeper::addReference(const char * const name, const void * pointer)
{
	if(keeper == nullptr) start();
	keeper->push(name,pointer);
}

void ThreadKeeper::delReference(const char * const name, const void * pointer)
{
	if(keeper == nullptr) start();
	keeper->pop(name,pointer);
}
const unsigned long ThreadKeeper::referenceCount()
{
	if(keeper == nullptr) start();
	return keeper->getCount();
}


	};
};
