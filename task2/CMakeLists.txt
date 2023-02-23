cmake_minimum_required(VERSION 3.10)
project(Rasterizer)
set(CMAKE_CXX_STANDARD 17)


FIND_PACKAGE(OpenCV REQUIRED
        PATHS ../dependencies/OpenCV2
        )
include_directories(${OpenCV_INCLUDE_DIRS})
find_package(Eigen3 REQUIRED
        PATHS ../dependencies/Eigen3)
include_directories(${EIGEN3_INCLUDE_DIRS})

add_executable(Rasterizer main.cpp rasterizer.hpp rasterizer.cpp Triangle.hpp Triangle.cpp global.hpp)

target_link_libraries(Rasterizer
        ${OpenCV_LIBS}
        )
