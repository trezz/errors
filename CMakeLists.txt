if (${APPLE})
    # CMake 3.15 added support of C++20 for apple clang
    cmake_minimum_required(VERSION 3.15)
else()
    # CMake 3.12 is the first release supporting C++20
    cmake_minimum_required(VERSION 3.12)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

project(errors VERSION 0.1.0)

add_executable(errors main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
