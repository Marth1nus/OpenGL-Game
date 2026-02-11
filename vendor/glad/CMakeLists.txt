cmake_minimum_required(VERSION 3.20)
project(glad VERSION 1.0.0 LANGUAGES C)

add_library(glad
  src/glad.c)
target_include_directories(glad
  PUBLIC include)
