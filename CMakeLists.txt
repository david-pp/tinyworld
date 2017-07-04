cmake_minimum_required(VERSION 2.8)
project(tinyworld)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Werror -Wall -g")

# INCLUDE DIR
include_directories(common)
include_directories(/usr/local/include)
include_directories(/usr/include/mysql)

# LINK DIR
link_directories(common)
link_directories(/usr/local/lib/)
link_directories(/usr/lib/)


set(LIBS
        tinyworld
        log4cxx apr-1 aprutil-1 iconv
        mysqlpp
        mysqlclient z ssl crypto
        boost_system boost_thread
        zmq
        protobuf)
#
#set(CMAKE_CXX_FLAGS "-v")
#set(CMAKE_EXE_LINKER_FLAGS "-v")

set(LIBS_BOOST boost_system boost_thread)
set(LIBS_MYSQL mysqlpp mysqlclient z ssl crypto)


#
#  SOCI ? Mysql++ ?
#
set(USE_ORM_SOCI 0)
set(ORM_LIBS "")
if (USE_ORM_SOCI)
    include_directories(/usr/local/include/soci
            /usr/local/include/soci/mysql
            /usr/include/mysq)
    link_directories(/usr/local/lib64)

    set(ORM_LIBS soci_core soci_mysql)
else ()
    add_definitions(-DUSE_ORM_MYSQLPP)
    set(ORM_LIBS mysqlpp)
endif ()

message("COMPILER:   ${CMAKE_CXX_COMPILER}")
message("CXXFLAGS: ${CMAKE_CXX_FLAGS}")
message("LDFLAGS : ${CMAKE_EXE_LINKER_FLAGS}")

add_subdirectory(common)
#add_subdirectory(dataserver)
add_subdirectory(example)
add_subdirectory(example/tinyobj)
add_subdirectory(test)
