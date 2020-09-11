project(etherCAT_client)

cmake_minimum_required(VERSION 3.0)

## use this to globally use C++14 with in our project
set(CMAKE_CXX_STANDARD 14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Xenomai QUIET)

find_package(yaml-cpp QUIET)

option(ENABLE_XENO FALSE)

find_package(Protobuf REQUIRED)

find_library(ecat_master_pb
    NAMES ecat_master_pb
    HINTS "${CMAKE_INSTALL_PREFIX}/lib"
)

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

        
set(COMMON_LIBS
    yaml-cpp
)

include_directories(${CMAKE_INSTALL_PREFIX}/include/ecat_master_pb)
include_directories(${PROJECT_SOURCE_DIR}/../../EtherCAT_Master/ecat_master_advr/include)

add_executable(ethercat_client ethercat_client.cpp)

## add the include directory to our compile directives
target_include_directories(ethercat_client PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(ethercat_client PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf ${ecat_master_pb})




add_executable(ethercat_client2 ethercat_client2.cpp)

## add the include directory to our compile directives
target_include_directories(ethercat_client2 PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(ethercat_client2 PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf ${ecat_master_pb})


add_executable(ethercat_client3 ethercat_client3.cpp)

## add the include directory to our compile directives
target_include_directories(ethercat_client3 PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(ethercat_client3 PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf ${ecat_master_pb})


add_executable(ethercat_client4 ethercat_client4.cpp)

## add the include directory to our compile directives
target_include_directories(ethercat_client4 PUBLIC ${ZeroMQ_INCLUDE_DIR})
## at the 0mq library to our link directive
target_link_libraries(ethercat_client4 PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf ${ecat_master_pb})


if(${ENABLE_XENO})
    add_executable(ethercat_client_RTNRT ethercat_client_RTNRT.cpp)
    ## add the include directory to our compile directives
    target_include_directories(ethercat_client_RTNRT PUBLIC ${ZeroMQ_INCLUDE_DIR})
    ## at the 0mq library to our link directive
    target_link_libraries(ethercat_client_RTNRT PUBLIC ${ZeroMQ_LIBRARY} -lprotobuf ${ecat_master_pb} -lyaml-cpp)
    set_xeno_flags(ethercat_client_RTNRT)
endif()
