#include <sstream>
#include <iterator>
#include <algorithm>
#include "utility/logging/logger.hpp"
#include "handler.hpp"
#include "handler_registrar.hpp"
#include "web_engine.hpp"
using namespace Cordite;
using namespace Cordite::Logging;
using std::endl;
namespace Cordite {
//---------------------------
/*!
 * Takes a Response object and outputs content to send over
 * the wire.
 *
 * Output will look like
 * 
 *     Code:404
 *	   Tag: someid-here
 *	   Type: BSON
 *     Header-Thing:Contents
 *     Header-Another:More content
 *	   Body:{Some json or binary content}
 */
std::list<std::string> ResponseToOutput(HandlerResponse & r)
{
	std::list<std::string> a;
	std::stringstream temp;
	temp << "Code:" << r.meta.code;
	a.push_back(temp.str());
	temp.clear();//clear any bits set
	temp.str(std::string());
	if(r.meta.tag.size() > 0)
	{
		temp << "Tag:" << r.meta.tag;
		a.push_back(temp.str());
		temp.clear();
		temp.str(std::string());
	}
	temp << "Type:";
	switch(r.meta.type)
	{
		case RRT_STRING:
		{
			temp << "string";
			break;
		}
		case RRT_BSON:
		{
			temp << "bson";
			break;
		}
		case RRT_MSGPACK:
		{
			temp << "msgpack";
			break;
		}
		case RRT_PROTOBUF:
		{
			temp << "protobuf";
			break;
		}
		case RRT_JSON:
		{
			temp << "json";
			break;
		}
		default:
		{
			temp << "UNKNOWN";
			break;
		}
	}
	a.push_back(temp.str());
	temp.clear();
	temp.str(std::string());
	for(auto h : r.meta.headers)
	{
		temp << "Header-" << h.first << ":";
		temp << h.second;
		a.push_back(temp.str());
		temp.clear();//clear any bits set
		temp.str(std::string());
	}
	temp << "Body:" << r.body;
	a.push_back(temp.str());
	temp.clear();//clear any bits set
	temp.str(std::string());
	return a;
}
/*!
 *	Takes an input of strings and presents a request object.
 *	The format of a request is expected to be
 *	
 *	* Type
 *	* URI
 *	* (optional) Tag (for response)
 *	* (optional) Auth
 *	* (optional) Version (for requests with body)
 *	* (repeatable) Header-Name
 *	* (optional) Body
 *	
 *	That is, the client sends the data as such
 *      Type:POST
 *	    URI:/users/user
 *	    Tag: someid-here
 *	    Version:BSON
 *	    Header-UserID:42
 *	    Body:SomeContentHere and Stuff :colons blah
 */
HandlerRequest InputToRequest(std::list<std::string> & in)
{
	//Note: This first verifies before it fully parses.
	HandlerRequest r;
	r.method = HM_NULL;
	if(in.front().compare(0, 5, "Type:") != 0)
	{
		//Invalid request
		return r;
	}
	auto method = in.front().substr(
			5,
			in.front().size() - 5
			);
	in.pop_front();
	//defer handling method until the end.
	if(in.front().compare(0,4,"URI:") != 0)
	{
		//Invalid request
		return r;
	}
	auto uri = in.front().substr(
			4,
			in.front().size() - 4
			);
	in.pop_front();
	//defer handling the URI until the end.
	if(in.front().compare(0,4,"Tag:") == 0)
	{
		r.tag = in.front().substr(
			4,
			in.front().size() - 4
			);
		in.pop_front();
	}
	std::string auth = "0";
	if(in.front().compare(0,5,"Auth:") == 0)
	{
		auth = in.front().substr(
			5,
			in.front().size() - 5
			);
		in.pop_front();
	}
	//defer handling the Auth until the end.
	std::string version = "NULL";
	if(in.front().compare(0,8,"Version:") == 0)
	{
		version = in.front().substr(
			8,
			in.front().size() - 8
			);
		in.pop_front();
	}
	//defer handling the Version until the end.
	int advanced = 0;
	for(auto h : in)
	{
		//See if this is even a header entry
		//We may (and should) hit the body and
		//break out here.
		if(h.compare(0,7,"Header-") != 0) break;
		auto header = h.substr(7,h.size()-7);
		auto found = header.find(":");
		if(found == std::string::npos) break;
		advanced++;
		auto name = header.substr(0, found);
		auto value = header.substr(
			found+1,
			header.size() - (found+1)
			);
		r.requestData[name] = value;
	}
	for(int i = 0; i < advanced; i++)
	{
		in.pop_front();
	}
	//We expect only a body at this point.
	if(in.size() > 0)
	{
		//A body is only one parameter
		//If we have more.. this is likely
		//a bad request!
		if(in.size() > 1) return r;
		if(in.front().compare(0,5,"Body:") != 0) return r;
		r.body = in.front().substr(5, in.front().size()-5);
	}
	//Now do the method and the URI
	if(method == "AUTH")
	{
		r.method = HM_AUTH;
	}
	else if(method == "OPTIONS")
	{
		r.method = HM_OPTIONS;
	}
	else if(method == "GET")
	{
		r.method = HM_GET;
	}
	else if(method == "POST")
	{
		r.method = HM_POST;
	}
	else if(method == "PUT")
	{
		r.method = HM_PUT;
	}
	else if(method == "DELETE")
	{
		r.method = HM_DELETE;
	}
	//Auth
	std::stringstream auths(auth);
	int au;
	auths >> au;
	r.authorization = (R_Authorization)au;
	//Version
	if(version == "NULL")
	{
		r.version = RSV_NULL;
	}
	else if(version == "BSON")
	{
		r.version = RSV_BSON;
	}
	else if(version == "MSGPACK")
	{
		r.version = RSV_MSGPACK;
	}
	else if(version == "PROTOBUF")
	{
		r.version = RSV_PROTOBUF;
	}
	//Lastly, the URI
	std::stringstream uris(uri);
	std::string part;
	while(std::getline(uris, part, '/'))
	{
		r.URIParts.push_back(part);
	}

	return r;
}
HandlerResponse MakeAuthorizationResponse(const R_Authorization auth)
{
	HandlerResponse r;
	switch(auth)
	{
		case RA_NULL:
		case RA_BANNED:
		{
			r.meta.code = RSC_ERR_FORBIDDEN;
			r.body = "0";
			break;
		}
		default:
		{
			r.meta.code = RSC_OK;
			std::stringstream s;
			s << (int)auth;
			r.body = s.str();
		}

	}
	return r;
}
std::list<std::string> WebEngine::evaluate(std::list<std::string> & msg)
{
	if(msg.size() == 1 && msg.front() == "DEMO")
	{
		std::list<std::string> a;
		a.push_back("DEMO-HAI");
		return a;
	}
	//Begin real processing.
	std::string tag;
	try
	{
		HandlerRequest req(InputToRequest(msg));
		if(req.method == HM_NULL)
		{
			HandlerResponse r;
			r.body = "Invalid request";
			r.meta.code = RSC_ERR_BAD_REQUEST;
			r.meta.tag = req.tag;
			return ResponseToOutput(r);
		}
		HandlerResponse res = evaluate(req);
		res.meta.tag = req.tag;
		tag = req.tag;
		return ResponseToOutput(res);
	}
	catch(std::exception & ex)
	{
		Logger::log(LGT_ERROR, [&ex,&msg](std::ostream & o){
			o << "Failure at evaluate!" << endl;
			o << "Message: " << ex.what() << endl;
			o << "Request was:" << endl;
			for(auto m : msg)
			{
				o << "* " << m << endl;
			}
			o << "End of failure.";
		});
	}
	catch(...)
	{
		Logger::log(LGT_ERROR, [&msg](std::ostream & o){
			o << "Unknown Failure at evaluate!" << endl;
			o << "Request was:" << endl;
			for(auto m : msg)
			{
				o << "* " << m << endl;
			}
			o << "End of failure.";
		});
	}
	//We are only here if a fatal error has happened
	HandlerResponse res;
	res.body = "There was an internal error while processing the request.";
	res.meta.code = RSC_SRV_ERR_INTERNAL;
	res.meta.tag = tag;
	return ResponseToOutput(res);
}
HandlerResponse WebEngine::evaluate(HandlerRequest & req)
{
	Handler * h = &HandlerRegistrar::getRoot();
	for(auto part : req.URIParts)
	{
		h = &h->resolve(part, &req);
	}
	HandlerResponse res;
	switch(req.method)
	{
		case HM_GET:
		{
			res = h->get(req);
			break;
		}
		case HM_POST:
		{
			res = h->post(req);
			break;
		}
		case HM_PUT:
		{
			res = h->put(req);
			break;
		}
		case HM_DELETE:
		{
			res = h->del(req);
			break;
		}
		case HM_OPTIONS:
		{
			res = h->get(req);
			break;
		}
		case HM_AUTH:
		{
			res = MakeAuthorizationResponse(
				h->authorize(req.body, req.version)
				);
			break;
		}
		default:
		{
			res.meta.code = RSC_ERR_BAD_METHOD;
			break;
		}
	}
	return res;
}

//---------------------------
};