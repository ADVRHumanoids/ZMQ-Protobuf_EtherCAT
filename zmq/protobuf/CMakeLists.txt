cmake_minimum_required(VERSION 3.2)
project(my_msg_pb)

find_package(Protobuf REQUIRED)
include_directories(${PROTOBUF_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(FindPkgConfig)

set(msgs
   my_msg.proto
)

set(LIB_NAME ${PROJECT_NAME})

add_definitions(-fPIC)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER ${msgs})
add_library (${LIB_NAME} ${PROTO_HEADER} ${PROTO_SRC})

install(TARGETS ${LIB_NAME} DESTINATION lib)
install(FILES ${PROTO_HEADER}  DESTINATION include/protobuf)
