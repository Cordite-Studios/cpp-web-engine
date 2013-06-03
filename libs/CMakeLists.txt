cmake_minimum_required(VERSION 2.8)
project(cordite-libs CXX)

add_subdirectory(bson)



include_directories(include ./)
#add_subdirectory(session)
add_subdirectory(utility)
