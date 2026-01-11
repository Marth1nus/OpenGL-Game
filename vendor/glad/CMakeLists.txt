cmake_minimum_required(VERSION 4.2)
project(glad)
add_library(${PROJECT_NAME} "src/glad.c")
target_include_directories(${PROJECT_NAME} PUBLIC "include/")