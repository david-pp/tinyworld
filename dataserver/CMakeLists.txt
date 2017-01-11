cmake_minimum_required(VERSION 3.3)

#file(GLOB SRCFILES *.cpp)
set(SRCFILES main.cpp object2db.h object2db.cpp object2db.in.h)
add_executable(dataserver ${SRCFILES})
target_link_libraries(dataserver ${LIBS})