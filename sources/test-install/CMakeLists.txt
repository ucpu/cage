cmake_minimum_required(VERSION 3.22)
project(cage-test-install CXX)

find_package(Cage REQUIRED)

add_executable(cage-test-install ../test-core/installConsistentPaths.cpp main.cpp)
target_link_libraries(cage-test-install cage-simple)
