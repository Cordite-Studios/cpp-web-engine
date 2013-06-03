#pragma once
#include <cstdlib>
#include <typeinfo>

namespace Cordite {
	namespace Memory {
		class ThreadKeeper {
		public:
			static void start();
			static void end();
			static void addReference(const char * const, const void *);
			static void delReference(const char * const, const void *);
			template<typename T>
			static void addReference(const void * p)
			{
				addReference(typeid(T).name(), p);
			}
			template<typename T>
			static void delReference(const void * p)
			{
				delReference(typeid(T).name(), p);
			}
			template<typename T>
			static void addReference(const T * t)
			{
				addReference(typeid(T).name(), (void *)t);
			}
			template<typename T>
			static void delReference(const T * t)
			{
				delReference(typeid(T).name(), (void *)t);
			}
			static const unsigned long referenceCount();
		};
		
		template<typename T>
		class Tracked {
		public:
			void * operator new(size_t size)
			{
				void * p = malloc(size);
				ThreadKeeper::addReference<T>(p);
				return p;
			}
			void operator delete(void * p)
			{
				ThreadKeeper::delReference<T>(p);
				free(p);
			}
		};
		class ScopedKeeper {
		public:
			ScopedKeeper()
			{
				ThreadKeeper::start();
			}
			~ScopedKeeper()
			{
				ThreadKeeper::end();
			}
		};
	};
};