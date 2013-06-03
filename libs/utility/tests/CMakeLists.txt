
add_executable(cordite-worker-test worker.cpp)
target_link_libraries(cordite-worker-test cordite-utility)
add_executable(cordite-log-mem-test logger_mem.cpp)
target_link_libraries(cordite-log-mem-test cordite-utility)
add_executable(cordite-log-test logger.cpp)
target_link_libraries(cordite-log-test cordite-utility)
add_executable(cordite-keeper-mixin-test keeper-mixin.cpp)
target_link_libraries(cordite-keeper-mixin-test cordite-utility)