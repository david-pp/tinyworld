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

add_executable(demo_reflection demo_reflection.cpp)

add_executable(demo_serialize demo_serialize.cpp ${PB_CPPOUTS})
target_link_libraries(demo_serialize tinyworld protobuf)

add_executable(demo_serialize_dyn demo_serialize_dyn.cpp ${PB_CPPOUTS})
target_link_libraries(demo_serialize_dyn tinyworld protobuf)