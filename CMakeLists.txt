cmake_minimum_required(VERSION 3.20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # For clang-tidy.
set(BUILD_SHARED_LIBS OFF) # We expect external libraries to be linked statically.
set(CMAKE_CXX_STANDARD 20) # Compile as C++20.
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

project(myCoroutine VERSION 0.1.0 LANGUAGES CXX)
include_directories(src/myCoroutine.hpp)
add_executable(myCoroutine main.cpp)
enable_testing()
