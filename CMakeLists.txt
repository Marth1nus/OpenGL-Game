cmake_minimum_required(VERSION 3.20)
project(OpenGL-Game VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Static linking of "Microsoft Visual C++ Redistributable"
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
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Emscripten Configuration
if(EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME ${PROJECT_NAME}
    SUFFIX .html)
  target_compile_options(${PROJECT_NAME} PRIVATE
    -sUSE_GLFW=3
    -sUSE_WEBGL2=1
    -sFULL_ES3=1
    -sEXPORTED_RUNTIME_METHODS=['setCanvasSize']
    -sEXCEPTION_CATCHING_ALLOWED=['*'])
  # Add npm run build using emsdk included node
  if($ENV{EMSDK_NODE} MATCHES " *")
    message(FATAL_ERROR "Missing emsdk environment variable(s)")
  else()
    get_filename_component(NODE_DIR $ENV{EMSDK_NODE} DIRECTORY)
    if(CMAKE_HOST_SYSTEM_NAME MATCHES "Windows")
      set(NPM_COMMAND ${NODE_DIR}/npm.cmd)
    else()
      set(NPM_COMMAND ${NODE_DIR}/npm)
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E env
      "PATH=${NODE_DIR}"
      "${NPM_COMMAND}" install
      COMMAND ${CMAKE_COMMAND} -E env
      "PATH=${NODE_DIR}"
      "ENV_CMAKE_PROJECT_NAME=$<TARGET_PROPERTY:${PROJECT_NAME},NAME>"
      "ENV_CMAKE_TARGET_FILE_DIR=$<TARGET_FILE_DIR:${PROJECT_NAME}>"
      "ENV_CMAKE_BUILD_CONFIG=$<CONFIG>"
      "${NPM_COMMAND}" run build
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web/"
      COMMENT "npm install && npm run build"
      USES_TERMINAL)
  endif()
endif()

# Link GLFW
if(EMSCRIPTEN)
  # GLFW Linked as part of Emscripten Configuration
else()
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(glfw)
  target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
endif()

# Link OpenGL ES 3.2
if(EMSCRIPTEN)
  # OpenGL Linked as part of Emscripten Configuration
elseif(MSVC)
  add_subdirectory(vendor/glad)
  target_link_libraries(${PROJECT_NAME} PRIVATE glad)
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE GLESv2 EGL)
endif()
