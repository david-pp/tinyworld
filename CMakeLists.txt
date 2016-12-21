cmake_minimum_required(VERSION 3.3)
project(tinyworld)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


# PROJECT
include_directories(common)

# MYSQL
include_directories(/usr/local/include)
include_directories(/usr/include/mysql)

link_directories(/usr/local/lib/)
link_directories(/usr/lib/)


set(LIBS log4cxx apr-1 aprutil-1 iconv)
#
#set(CMAKE_CXX_FLAGS "-v")
#set(CMAKE_EXE_LINKER_FLAGS "-v")

message("COMPILER:   ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS: ${CMAKE_CXX_FLAGS}")
message("LDFLAGS : ${CMAKE_EXE_LINKER_FLAGS}")

add_subdirectory(common)
add_subdirectory(dataserver)
