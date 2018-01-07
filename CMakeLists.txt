cmake_minimum_required(VERSION 3.8)
project(node2vec)

set(CMAKE_CXX_STANDARD 98)

# 开启啰嗦模式
set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -O3 -DNDEBUG -fopenmp")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -std=c++98 -Wall -O3 -DNDEBUG -fopenmp")

# 添加依赖库
add_subdirectory(snap-core)

#set(CMAKE_EXE_LINKER_FLAGS "-lpthread -lrt")
set(EXSNAPADV ./snap-adv)

set(DEPH ${EXSNAPADV}/n2v.h ${EXSNAPADV}/word2vec.h ${EXSNAPADV}/biasedrandomwalk.h)
set(DEPCPP ${EXSNAPADV}/n2v.cpp ${EXSNAPADV}/word2vec.cpp ${EXSNAPADV}/biasedrandomwalk.cpp)

include_directories(./snap-adv)
include_directories(./snap-core)
include_directories(./snap-exp)
include_directories(./glib-core)
include_directories(./glib-adv)

aux_source_directory(./examples/node2vec DIR_SRCS)

add_executable(node2vec ${DIR_SRCS} ${DEPH} ${DEPCPP})
target_link_libraries(node2vec snap)
