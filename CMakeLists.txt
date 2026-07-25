cmake_minimum_required(VERSION 3.16)
project(ShamelaOpener VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)


add_executable(${PROJECT_NAME} main.cpp theme.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    Qt6::Network
)