cmake_minimum_required(VERSION 3.0.0)
project(libsnap VERSION 2.4)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -O3 -DNDEBUG -fopenmp -g -rdynamic -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -O3 -DNDEBUG -fopenmp")

## lib
add_library(snap SHARED ./Snap.cpp)
target_include_directories(snap PUBLIC ../glib-core ../snap-core)


add_executable(testSnap ../snap-core/testSnap.cpp)
target_link_libraries(testSnap snap)

