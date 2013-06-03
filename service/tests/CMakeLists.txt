#Things for tests.
add_executable(cordite-test-worker-group worker_group.cpp)
target_link_libraries(cordite-test-worker-group cservice)
add_executable(cordite-test-web-engine web_engine.cpp)
target_link_libraries(cordite-test-web-engine cservice)
add_executable(cordite-test-dispatcher dispatcher.cpp)
target_link_libraries(cordite-test-dispatcher cservice)