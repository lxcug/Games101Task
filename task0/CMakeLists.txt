project(task0)
cmake_minimum_required(VERSION 3.17)
set(CMAKE_CXX_STANDARD 20)

find_package(Eigen3 REQUIRED
        PATHS ../dependencies/Eigen3)
include_directories(${EIGEN3_INCLUDE_DIRS})


add_executable(task0 main.cpp)
