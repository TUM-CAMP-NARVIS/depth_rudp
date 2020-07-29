cmake_minimum_required(VERSION 3.2)
project(DepthTest VERSION 1.0.0 LANGUAGES CXX)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -pthread")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

file(GLOB SOURCE src/*cpp)

add_library(depth_rudp SHARED ${SOURCE})
target_include_directories(depth_rudp PUBLIC include)
target_link_libraries(depth_rudp PRIVATE ${CONAN_LIBS})

add_executable(depth_enet_server examples/DepthSender.cpp)
target_link_libraries(depth_enet_server PRIVATE depth_rudp stdc++fs ${CONAN_LIBS})

add_executable(depth_enet_client examples/DepthReceiver.cpp)
target_link_libraries(depth_enet_client PRIVATE depth_rudp stdc++fs ${CONAN_LIBS})

add_executable(example_basic examples/example_basic.cpp)
target_link_libraries(example_basic PRIVATE depth_rudp stdc++fs ${CONAN_LIBS})

