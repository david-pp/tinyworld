cmake_minimum_required(VERSION 3.3)

add_executable(tinyobj demo_tinyobj.cpp tinyobj.cpp tinyplayer.cpp tinyplayer.pb.cc)
target_link_libraries(tinyobj tinyworld protobuf ${ORM_LIBS})
