cmake_minimum_required(VERSION 3.17)
project(Multithreaded_Matrix_Multiplication C)

set(CMAKE_C_STANDARD 11)

add_executable(Multithreaded_Matrix_Multiplication main.c)

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} Threads::Threads)