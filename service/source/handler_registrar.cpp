#include <iostream>
#include "handler_registrar.hpp"
#include "handler.hpp"

namespace Cordite {
//----------------------
static HandlerRegistrar * hr = nullptr;
class HRWatcher {
public:
	~HRWatcher()
	{
		if(hr) delete hr;
	}
} HRKillOnExit;
class HRInternals {
public:
	std::map<std::string, Handler *> handles;
	std::multimap<std::string, Handler *> toRegister;
	bool has_root;
	Handler * rootHandler = nullptr;
	std::string originalRootName;
	HRInternals()
	: has_root(false), rootHandler(nullptr)
	{

	}
};

HandlerRegistrar::HandlerRegistrar()
{
	_ = new HRInternals;
}
HandlerRegistrar::~HandlerRegistrar()
{
	if(_) delete _;
}
HandlerRegistrar & HandlerRegistrar::get()
{
	if(!hr)
	{
		hr = new HandlerRegistrar();
	}
	return *hr;
}

void HandlerRegistrar::registerHandle(const std::string name, Handler * handle)
{
	if(handle == nullptr) return;
	if(name.size() == 0) return;
	if(name == "void") return;
	if(*(handle->handle())=='\t')
	{
		//Who the crap tried to register the null handler..?
		return;
	}
	if(*(handle->handle())==0)
	{
		//We are the root!
		if(_->has_root)
		{
			//Errr.. We already have one..?
			std::cerr << "WARNING: Root Handler already registered!" << std::endl;
			std::cerr << "\tOriginal: " << _->originalRootName << std::endl;
			std::cerr << "\tCurrent: " << name << std::endl;
			std::cerr << "Current Handler will not be registered!" << std::endl;
			std::cerr << std::endl;
			//We shouldn't exactly throw an exception.. 
			return;
		}
		else
		{
			_->has_root = true;
			_->originalRootName = name;
			_->rootHandler = handle;
		}
	} 
	_->handles[name] = handle;
}
void HandlerRegistrar::registerTo(const std::string name, Handler * handle)
{
	if(handle == nullptr) return;
	if(name.size() == 0) return;
	if(name == "void") return;
	_->toRegister.insert(std::pair<std::string, Handler *>(name,handle));
}
void HandlerRegistrar::init()
{
	auto _ = get()._;
	for(auto handlerPair : _->toRegister)
	{
		_->handles[handlerPair.first]->addHandler(handlerPair.second);
	}
}
Handler & HandlerRegistrar::getRoot()
{
	return *(get()._->rootHandler);
}

//----------------------
};