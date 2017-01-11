cmake_minimum_required(VERSION 3.3)

#file(GLOB SRCFILES *.cpp)
set(SRCFILES mydb.cpp url.cpp)
add_library(tinyworld STATIC ${SRCFILES})