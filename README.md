# ZMQ-Protobuf_EtherCAT

# ZMQ
ZeroMQ (also spelled ØMQ, 0MQ or ZMQ) is a high-performance asynchronous messaging library, aimed at use in distributed or concurrent applications. It provides a message queue, but unlike message-oriented middleware, a ZeroMQ system can run without a dedicated message broker.

More information here: [ZMQ Information](https://zeromq.org/)

## Installation  
Build steps:

Build [libzmq](https://github.com/zeromq/libzmq) via cmake. This does an out of source build and installs the build files download and unzip the lib, cd to directory:
```
mkdir build
cd build
cmake ..
sudo make -j4 install
```
        
Build [cppzmq](https://github.com/zeromq/cppzmq) via cmake. This does an out of source build and installs the build files
download and unzip the lib, cd to directory:
```
mkdir build
cd build
cmake ..
sudo make -j4 install
```


Here more details: [ZMQ Installation](https://github.com/zeromq/cppzmq)

# Protobuf (Protocol Buffer)
Protocol buffers are a flexible, efficient, automated mechanism for serializing structured data – think XML, but smaller, faster, and simpler. You define how you want your data to be structured once, then you can use special generated source code to easily write and read your structured data to and from a variety of data streams and using a variety of languages. You can even update your data structure without breaking deployed programs that are compiled against the "old" format. 

More information here: [Protobuf Information](https://developers.google.com/protocol-buffers)

## Installation  
Build steps:

```
sudo apt install protobuf-compiler

```

# Additional Installation

```
sudo apt install libboost-all-dev libzmq5 libyaml-cpp-dev libjsoncpp-dev libprotobuf-dev

```

# EtherCAT Integration

## Server
Using the superbuild is possible to activate ec_master_test, ec_master_advr and soem_advr repositories:

![firstImage](https://github.com/ADVRHumanoids/ZMQ-Protobuf_EtherCAT/blob/master/img/ec_master_advr%26test.png)

![secondImage](https://github.com/ADVRHumanoids/ZMQ-Protobuf_EtherCAT/blob/master/img/soem_advr.png)

Here the repositories:

[https://gitlab.advr.iit.it/xeno-ecat/ec_master_test](https://gitlab.advr.iit.it/xeno-ecat/ec_master_test)

Note: Use this branch MultiDOF-superBuildInt

[https://gitlab.advr.iit.it/xeno-ecat/ecat_master_advr](https://gitlab.advr.iit.it/xeno-ecat/ecat_master_advr)

[https://gitlab.advr.iit.it/xeno-ecat/soem_advr](https://gitlab.advr.iit.it/xeno-ecat/soem_advr)

## Client




