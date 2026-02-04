cmake_minimum_required(VERSION 3.16)
project(MyCppProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
    src/main.cpp
)

target_include_directories(app PRIVATE include)
