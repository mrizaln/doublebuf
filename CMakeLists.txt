cmake_minimum_required(VERSION 3.12)

project(
  doublebuf
  VERSION 0.2.0
  LANGUAGES CXX
  DESCRIPTION "A simple lock-less double buffering mechanism written in C++20"
)

string(
  COMPARE EQUAL
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}"
  DOUBLEBUF_STANDALONE
)

option(DOUBLEBUF_BUILD_EXAMPLES "Build examples" ${DOUBLEBUF_STANDALONE})

add_library(doublebuf INTERFACE)
target_include_directories(doublebuf INTERFACE include)
target_compile_features(doublebuf INTERFACE cxx_std_20)
set_target_properties(doublebuf PROPERTIES CXX_EXTENSIONS OFF)

if(DOUBLEBUF_BUILD_EXAMPLES)
  add_subdirectory(example)
endif()
