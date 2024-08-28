cmake_minimum_required(VERSION 3.12)

project(
  doublebuf
  VERSION 0.0.0
  LANGUAGES CXX
  DESCRIPTION
    "A simple lock-less double buffering mechanism implementation written in C++20"
)

add_library(doublebuf INTERFACE)
target_include_directories(doublebuf INTERFACE include)
target_compile_features(doublebuf INTERFACE cxx_std_20)
set_target_properties(doublebuf PROPERTIES CXX_EXTENSIONS OFF)
