cmake_minimum_required(VERSION 3.16)
project(ShamelaOpener VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Find Qt (supports both Qt5 and Qt6)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# قمنا بإضافة theme.cpp هنا ليتم بناؤه مع المشروع
add_executable(${PROJECT_NAME} main.cpp theme.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)


find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
target_link_libraries(ShamelaOpener PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)