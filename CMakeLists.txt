cmake_minimum_required(VERSION 2.8)
project(cordite CXX)
set(CMAKE_BUILD_TYPE Debug)
if (APPLE)
	include_directories(/usr/local/include)
	link_directories(/usr/local/lib)
	set(CMAKE_XCODE_ATTRIBUTE_GCC_VERSION "com.apple.compilers.llvm.clang.1_0")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
	
	#these have absolutely no effect :(
	if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
		message(FATAL_ERROR "You need to use clang on mac!
		use CC=clang CXX=clang++ cmake path/to/project")
	endif()
	#SET(CMAKE_C_COMPILER "clang")
	#SET(CMAKE_CXX_COMPILER "clang++")
	
	SET(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ -g")
	#message(NOTICE ${CMAKE_CXX_FLAGS})
elseif (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
  set ( CMAKE_CXX_FLAGS "-std=c++0x -g")
  message(FATAL_ERROR "You need to use clang. GCC sucks at thread local storage
		use CC=clang CXX=clang++ cmake path/to/project")
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	#Not apple, but clang.. 
	#include_directories("/usr/include/c++/4.7" "/usr/include/x86_64-linux-gnu/c++/4.7/")
	SET(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -lcxxrt -ldl")
	include_directories("/usr/include/c++/v1")
	SET(CMAKE_CXX_FLAGS "-std=c++11 -g -stdlib=libc++")
endif ()

add_subdirectory(libs)
add_subdirectory(service)