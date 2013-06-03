#include <iostream>
#include "web_engine.hpp"
#include "handler_registrar.hpp"
#include "handler.hpp"
#include "utility/logging/logger.hpp"

using namespace Cordite;
using namespace Cordite::Logging;
using namespace std;
int main()
{
	ScopedLogger l;
	Logger::output(LGO_CERR, LGT_INFO);
	HandlerRegistrar::init();
	Handler::Request req;
	req.URIParts = {"demo"};
	req.method = HM_GET;
	Handler::Response res = WebEngine::evaluate(req);
	Logger::log(LGT_INFO, [&](ostream & o){
		o << "Got a direct response!" << endl;
		o << "Code: " << res.meta.code << endl;
		o << "Body: " << res.body << endl;
	});
	req.URIParts = {"demo", "hello there"};
	res = WebEngine::evaluate(req);
	Logger::log(LGT_INFO, [&](ostream & o){
		o << "Got a direct response!" << endl;
		o << "Code: " << res.meta.code << endl;
		o << "Body: " << res.body << endl;
	});
	
}