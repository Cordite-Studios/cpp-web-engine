#pragma once
#include <type_traits>
#include "handler_registrar.hpp"

namespace Cordite {
	template<class TR>
	struct IF_REG {
		static bool DoRegister()
		{
			return true;
		}
	};
	template<>
	struct IF_REG<void>
	{
		static bool DoRegister()
		{
			return false;
		}
	};
		
	template<class T, class... ToRegister>
	class HandlerRegister {
		static_assert(std::is_default_constructible<T>::value
			|| std::is_trivially_default_constructible<T>::value,
			"The Handler needs to be trivially constructable");
	private:
		T * handle;

		template<class TR = void, class ...TRs>
		void DoRegister()
		{

			if(IF_REG<TR>::DoRegister()){
				HandlerRegistrar::get().registerTo(typeid(TR).name(), handle);
			}
			if(sizeof...(TRs))
			{
				DoRegister<TRs...>();
			}
		}

	public:
		HandlerRegister() {
			handle = new T();
			HandlerRegistrar::get().registerHandle(typeid(T).name(), handle);
			if(sizeof...(ToRegister))
			{
				DoRegister<ToRegister...>();
			}
		}
		~HandlerRegister() {
			delete handle;
		}
	};
};