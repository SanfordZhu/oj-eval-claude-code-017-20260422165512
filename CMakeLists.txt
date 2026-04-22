cmake_minimum_required(VERSION 3.10)
project(problem_017)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code code.cpp)

# Optimize for performance
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(code PRIVATE -O2 -pipe -static -s)
endif()
