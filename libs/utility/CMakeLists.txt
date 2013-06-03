
include_directories(include)
LIST(APPEND UTIL_HEADERS
	../include/utility/memory/keeper.hpp
	../include/utility/logging/logger.hpp
	../include/utility/networking/zeromq.hpp
	../include/utility/worker.hpp
)

LIST(APPEND UTIL_SOURCES
	keeper.cpp
	logger.cpp
	zeromq.cpp
	worker.cpp
)

add_library(cordite-utility SHARED ${UTIL_SOURCES} ${UTIL_HEADERS})
set_property(TARGET cordite-utility PROPERTY COMPILE_FLAGS "-fPIC")
target_link_libraries(cordite-utility zmq)
add_subdirectory(tests)