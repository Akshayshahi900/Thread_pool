cmake_minimum_required(VERSION 3.20)

project(ThreadPool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(server
    src/main.cpp
    src/threadPool.cpp
)

target_include_directories(server PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -g
)

target_link_libraries(server PRIVATE pthread)
