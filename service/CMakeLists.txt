project(cordite-service)

include_directories(
	include
	"${cordite-libs_SOURCE_DIR}/include"
	"${cordite-libs_SOURCE_DIR}/bson/inc"
) 

LIST(APPEND CSERVICE_SRC
	source/handler.cpp
	source/handler_registrar.cpp
	source/root.cpp
	source/dispatcher.cpp
	source/worker_group.cpp
	source/web_engine.cpp
)
LIST(APPEND CSERVICE_HEADERS
	include/dispatcher.hpp
	include/handler.hpp
	include/response.hpp
	include/authorization.hpp
	include/session_version.hpp
	include/handler_registrar.hpp
	include/handler_register.hpp
)
SET(CSERVICE_CORE ${CSERVICE_SRC} ${CSERVICE_HEADERS})

set(CSERVICE_SERVICES "")
add_subdirectory(source/services)

add_library(cservice SHARED ${CSERVICE_CORE} ${CSERVICE_SERVICES})
set_property(TARGET cservice PROPERTY COMPILE_FLAGS "-fPIC")
target_link_libraries(cservice cordite-utility)

add_executable(cserviced source/main.cpp)
target_link_libraries(cserviced cservice)
add_subdirectory(tests)