cmake_minimum_required(VERSION 3.20)
project(OpenGL-Game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD #[[            =  ]] 23)
set(CMAKE_CXX_EXTENSIONS #[[          = ]] OFF)
set(CMAKE_CXX_STANDARD_REQUIRED #[[   =  ]] ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS #[[ =  ]] ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY #[[    =  ]] "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(VENDOR_FETCH_DIR #[[              =  ]] "${CMAKE_CURRENT_SOURCE_DIR}/vendor-fetch" CACHE STRING "Vendor Code Downloads Folder")
set(GLFW_BUILD_X11 #[[                = ]] OFF CACHE BOOL "Disable GLFW X11 backend")
set(GLFW_BUILD_XKB #[[                = ]] OFF CACHE BOOL "Disable XKB (X11-only) support")
set(GLFW_BUILD_WAYLAND #[[            =  ]] ON CACHE BOOL "Enable Wayland backend")

# Enable Warnings
add_library(enable_warnings INTERFACE)
if(MSVC)
  target_compile_options(enable_warnings INTERFACE /W4 /WX)
  target_compile_definitions(enable_warnings INTERFACE _CRT_SECURE_NO_WARNINGS)
else()
  target_compile_options(enable_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

add_subdirectory(vendor)
add_subdirectory(engine)
add_subdirectory(game)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME}
  PRIVATE enable_warnings engine game)

# Emscripten Configurations
if(EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX .mjs)
  target_link_options(${PROJECT_NAME} PRIVATE
    -sEXPORTED_RUNTIME_METHODS=['FS','setCanvasSize','requestFullscreen']
    -sEXCEPTION_CATCHING_ALLOWED=['*']
    -sEXPORT_NAME='createModule'
    -sMODULARIZE=1
    -sEXPORT_ES6=1)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/src-web/index.html $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying index.html output files to $<TARGET_FILE_DIR:${PROJECT_NAME}>"
  )
endif()
