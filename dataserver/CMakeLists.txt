cmake_minimum_required(VERSION 3.3)

#file(GLOB SRCFILES *.cpp)
set(SRCFILES main.cpp)
add_executable(dataserver ${SRCFILES})
target_link_libraries(dataserver ${LIBS})