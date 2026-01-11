cmake_minimum_required(VERSION 3.16)
project(OpenGL-Game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(CMAKE_CXX_FLAGS_DEBUG #[[   ]] "${CMAKE_CXX_FLAGS_DEBUG}   /MTd")
  set(CMAKE_C_FLAGS_DEBUG #[[     ]] "${CMAKE_C_FLAGS_DEBUG}     /MTd")
  set(CMAKE_CXX_FLAGS_RELEASE #[[ ]] "${CMAKE_CXX_FLAGS_RELEASE} /MT ")
  set(CMAKE_C_FLAGS_RELEASE #[[   ]] "${CMAKE_C_FLAGS_RELEASE}   /MT ")
endif()

include(FetchContent)
FetchContent_Declare(glm #[[   ]] GIT_REPOSITORY https://github.com/g-truc/glm #[[      ]] GIT_TAG 1.0.3 #[[   ]] SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm #[[   ]])
FetchContent_Declare(glfw #[[  ]] GIT_REPOSITORY https://github.com/glfw/glfw #[[       ]] GIT_TAG 3.4 #[[     ]] SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glfw #[[  ]])
FetchContent_MakeAvailable(glm)

add_executable(${PROJECT_NAME}
  src/main.cpp
)
target_link_libraries(${PROJECT_NAME} PRIVATE glm::glm)
target_include_directories(${PROJECT_NAME} PUBLIC "include/")

# Enable warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Emscripten")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Link GLFW
if(CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
  message(FATAL_ERROR "Emscripten not configured")
else()
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

# Link OpenGL ES 3.2
if(CMAKE_CXX_COMPILER_ID MATCHES "Emscripten")
elseif(WIN32)
  add_definitions(-DUSE_GLAD)
  add_subdirectory(vendor/glad)
  target_link_libraries(${PROJECT_NAME} PRIVATE glad)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE GLESv2 EGL)
endif()
