cmake_minimum_required(VERSION 3.3)

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
endforeach(PB ${PROTOS})



#file(GLOB SRCFILES *.cpp)
set(SRCFILES main.cpp object2db.h object2db.cpp object2db.in.h)


add_executable(dataserver ${SRCFILES})
target_link_libraries(dataserver ${LIBS})


add_executable(client client.cpp ${PB_CPPOUTS})
target_link_libraries(client ${LIBS})


add_executable(server server.cpp ${PB_CPPOUTS})
target_link_libraries(server tinyworld zmq protobuf )

add_executable(test_player test_player.cpp object2db.cpp ${PB_CPPOUTS})
target_link_libraries(test_player ${LIBS})