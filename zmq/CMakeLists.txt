project(zmq)

cmake_minimum_required(VERSION 3.0)

## use this to globally use C++14 with in our project
set(CMAKE_CXX_STANDARD 14)

find_package(Protobuf REQUIRED)
add_subdirectory(protobuf/)

## load in pkg-config support
find_package(PkgConfig)
## use pkg-config to get hints for 0mq locations
pkg_check_modules(PC_ZeroMQ QUIET zmq)

## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.hpp
        PATHS ${PC_ZeroMQ_INCLUDE_DIRS}
        )

## use the hint from about to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
        )

include_directories(${CMAKE_CURRENT_BINARY_DIR}/protobuf)

add_executable(zmq_client zmq_client.cpp)
add_executable(zmq_server zmq_server.cpp)
add_executable(cppzmq_client cppzmq_client.cpp)
add_executable(cppzmq_server cppzmq_server.cpp)
add_executable(zmqprotobuf_client zmqprotobuf_client.cpp)
add_executable(zmqprotobuf_server zmqprotobuf_server.cpp)



## add the include directory to our compile directives
target_include_directories(zmq_client PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(zmq_client PUBLIC ${ZeroMQ_LIBRARY})

## add the include directory to our compile directives
target_include_directories(zmq_server PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(zmq_server PUBLIC ${ZeroMQ_LIBRARY})

## add the include directory to our compile directives
target_include_directories(cppzmq_client PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(cppzmq_client PUBLIC ${ZeroMQ_LIBRARY})

## add the include directory to our compile directives
target_include_directories(cppzmq_server PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(cppzmq_server PUBLIC ${ZeroMQ_LIBRARY})


## add the include directory to our compile directives
target_include_directories(zmqprotobuf_client PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(zmqprotobuf_client PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf my_msg_pb)

## add the include directory to our compile directives
target_include_directories(zmqprotobuf_server PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(zmqprotobuf_server PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf my_msg_pb)