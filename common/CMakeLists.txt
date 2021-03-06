cmake_minimum_required(VERSION 3.3)

set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required

set(PROTOC /usr/local/bin/protoc)
set(PROTOS_DIR          ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOS_CPPOUT_DIR   ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE PROTOS *.proto)

#message("${PROTOS}")

foreach(PB ${PROTOS})
    get_filename_component(PB_NAME ${PB} NAME_WE)
    set(PB_H   ${PB_NAME}.pb.h)
    set(PB_CPP ${PB_NAME}.pb.cc)

    set(PB_CPPOUT
            ${PROTOS_CPPOUT_DIR}/${PB_H}
            ${PROTOS_CPPOUT_DIR}/${PB_CPP})

    set(PB_CPPOUTS
            ${PB_CPPOUTS}
            ${PB_CPPOUT})

    add_custom_command(
            OUTPUT  ${PB_CPPOUT}
            COMMAND ${PROTOC} -I=${PROTOS_DIR} --cpp_out=${PROTOS_CPPOUT_DIR} ${PB}
            DEPENDS  ${PB})

    message("${PROTOC} -I=${PROTOS_DIR} --cpp_out=${PROTOS_CPPOUT_DIR} ${PB}")

endforeach(PB ${PROTOS})

#file(GLOB SRCFILES *.cpp)
set(SRCFILES tinymysql.cpp url.cpp tinyrpc.cpp tinyorm.cpp redis.cpp redis_cmd.cpp eventloop.cpp async.cpp ${PB_CPPOUTS})
add_library(tinyworld STATIC ${SRCFILES})