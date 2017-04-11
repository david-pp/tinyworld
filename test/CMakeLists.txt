cmake_minimum_required(VERSION 3.3)

set(PROTOC /usr/local/bin/protoc)
set(PROTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOS_CPPOUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PROTOS *.proto)

#message("${PROTOS}")

foreach (PB ${PROTOS})
    get_filename_component(PB_NAME ${PB} NAME_WE)
    set(PB_H ${PB_NAME}.pb.h)
    set(PB_CPP ${PB_NAME}.pb.cc)

    set(PB_CPPOUT
            ${PROTOS_CPPOUT_DIR}/${PB_H}
            ${PROTOS_CPPOUT_DIR}/${PB_CPP})

    set(PB_CPPOUTS
            ${PB_CPPOUTS}
            ${PB_CPPOUT})

    add_custom_command(
            OUTPUT ${PB_CPPOUT}
            COMMAND ${PROTOC} -I=${PROTOS_DIR} --cpp_out=${PROTOS_CPPOUT_DIR} ${PB}
            DEPENDS ${PB})
endforeach (PB ${PROTOS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")

#add_definitions(-DTINYLOGGER_LOG4CXX)

add_definitions(-DCATCH_PLATFORM_WINDOWS)

add_executable(test_logger test_logger.cpp)
target_link_libraries(test_logger log4cxx apr-1 aprutil-1 iconv)


add_executable(test_msg test_msg.cpp ${PB_CPPOUTS})
target_link_libraries(test_msg tinyworld protobuf)

add_executable(test_zmq test_zmq.cpp ${PB_CPPOUTS})
target_link_libraries(test_zmq tinyworld protobuf zmq)

add_executable(test_rpc test_rpc.cpp ${PB_CPPOUTS})
target_link_libraries(test_rpc tinyworld protobuf zmq)

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

add_executable(test_orm test_orm.cpp)
target_link_libraries(test_orm tinyworld ${ORM_LIBS})

add_executable(test_reflection test_reflection.cpp)

add_executable(test_serialize test_serialize.cpp ${PB_CPPOUTS})
target_link_libraries(test_serialize tinyworld protobuf)

#
#add_executable(test_zmq  test.cpp)
#target_link_libraries(test_zmq zmq boost_thread boost_system)
