cmake_minimum_required(VERSION 3.16)
project(ciel-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

add_executable(ciel-client
    src/main.cpp
)

target_link_libraries(ciel-client PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)

target_compile_options(ciel-client PRIVATE
    -Wall
    -Wextra
    -Werror
)
