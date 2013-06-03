#include <iostream>
#include "utility/memory/keeper.hpp"
#include "utility/logging/logger.hpp"

using namespace Cordite::Memory;
using namespace Cordite::Logging;

//mixin class
class Thing : public Tracked<Thing> {
	
};

//Don't put a scoped logger in the global namespace!
//It won't destruct before the context tries to!
int main()
{
	ScopedLogger log;
	ScopedKeeper k;
	//Logger::output(LGO_CERR, LGT_WARN);
	ThreadKeeper::start();
	Thing * b = new Thing();
	
	std::cout << "Hai " << std::endl;
	//delete b;
	ThreadKeeper::end();
}