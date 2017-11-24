cmake_minimum_required(VERSION 3.3)

set(PROTOC /usr/local/bin/protoc)
set(PROTOS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(PROTOS_CPPOUT_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PROTOS *.proto)

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

add_executable(demo_msg demo_msg.cpp ${PB_CPPOUTS})
target_link_libraries(demo_msg tinyworld protobuf)

add_executable(demo_zmq demo_zmq.cpp ${PB_CPPOUTS})
target_link_libraries(demo_zmq tinyworld protobuf zmq)

add_executable(demo_rpc demo_rpc.cpp ${PB_CPPOUTS})
target_link_libraries(demo_rpc tinyworld protobuf zmq)

add_executable(demo_orm demo_orm.cpp ${PB_CPPOUTS})
target_link_libraries(demo_orm tinyworld protobuf ${ORM_LIBS})

add_executable(demo_tt demo_tt.cpp ${PB_CPPOUTS})
target_link_libraries(demo_tt tinyworld protobuf zmq ${ORM_LIBS})

add_executable(demo_fsm demo_fsm.cpp)

add_executable(demo_redis demo_redis.cpp)
target_link_libraries(demo_redis tinyworld ev hiredis)

