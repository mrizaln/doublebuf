cmake_minimum_required(VERSION 3.16)
project(doublebuf_test VERSION 0.0.0)

include(cmake/prelude.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(fmt REQUIRED)

add_subdirectory(lib/doublebuf)

add_executable(main source/main.cpp)
target_include_directories(main PRIVATE source)
target_link_libraries(main PRIVATE fmt::fmt doublebuf)
target_compile_options(main PRIVATE -Wall -Wextra -Wconversion -Wswitch-enum)

# sanitizer
target_compile_options(main PRIVATE -fsanitize=thread)
target_link_options(main PRIVATE -fsanitize=thread)
