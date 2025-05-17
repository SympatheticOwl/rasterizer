cmake_minimum_required(VERSION 3.29)
set(CMAKE_C_STANDARD 23)
set(PROJECT_NAME "rasterizer")
include_directories(/usr/bin/include)

project(${PROJECT_NAME} C)

find_package(PNG REQUIRED)

ADD_LIBRARY(LibsModule
        uselibpng.c
)

add_executable(${PROJECT_NAME} main.c)

target_link_libraries(${PROJECT_NAME} png)
target_link_libraries(${PROJECT_NAME} LibsModule)
