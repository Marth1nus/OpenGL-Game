cmake_minimum_required(VERSION 3.20)
project(OpenGL-Game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(MSVC) # Static linking of "Microsoft Visual C++ Redistributable"
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)
set(VENDOR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor)
FetchContent_Declare(glm #[[   ]] GIT_REPOSITORY https://github.com/g-truc/glm #[[      ]] GIT_TAG 1.0.3 #[[   ]] SOURCE_DIR ${VENDOR_DIR}/glm #[[   ]])
FetchContent_Declare(glfw #[[  ]] GIT_REPOSITORY https://github.com/glfw/glfw #[[       ]] GIT_TAG 3.4 #[[     ]] SOURCE_DIR ${VENDOR_DIR}/glfw #[[  ]])

FetchContent_MakeAvailable(glm)
if(MSVC)
  add_subdirectory(vendor/glad)
endif()
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_WAYLAND ON CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 OFF CACHE BOOL "" FORCE)
if(NOT EMSCRIPTEN)
  FetchContent_MakeAvailable(glfw)
endif()

# Enable Warnings
add_library(enable_warnings INTERFACE)
if(MSVC)
  target_compile_options(enable_warnings INTERFACE /W4 /WX)
  target_compile_definitions(enable_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(enable_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(engine)
add_subdirectory(game)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}
  PRIVATE enable_warnings engine game)

# Emscripten Configurations
if(EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX .html)
  target_link_options(${PROJECT_NAME} PRIVATE
    -sEXPORTED_RUNTIME_METHODS=['setCanvasSize']
    -sEXCEPTION_CATCHING_ALLOWED=['*'])
  set(WEB_PUBLIC_DIR ${CMAKE_SOURCE_DIR}/web/public)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WEB_PUBLIC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.html ${WEB_PUBLIC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.wasm ${WEB_PUBLIC_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:${PROJECT_NAME}>/${PROJECT_NAME}.js ${WEB_PUBLIC_DIR}
    COMMENT "Copying Emscripten output files to ${WEB_PUBLIC_DIR}"
  )
endif()
