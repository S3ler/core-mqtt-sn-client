cmake_minimum_required(VERSION 3.6)
project(core-mqtt-sn-client)

set(CMAKE_CXX_STANDARD 11)

include_directories(src)
include_directories(main)
include_directories(Implementation)


set(CORE_FILES
        src/core_defines.h
        src/global_defines.h
        src/mqttsn_messages.h
        src/SocketInterface.cpp
        src/SocketInterface.h
        src/MqttSnMessageHandler.cpp
        src/MqttSnMessageHandler.h
        src/Client.cpp
        src/Client.h
        )

add_library(core-mqtt-sn-client ${CORE_FILES})

