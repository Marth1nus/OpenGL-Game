include(FetchContent)
file(MAKE_DIRECTORY ${VENDOR_FETCH_DIR})
macro(macro_FetchContent_Declare NAME URL TAG)
  FetchContent_Declare(${NAME} GIT_REPOSITORY ${URL} GIT_TAG ${TAG} SOURCE_DIR "${VENDOR_FETCH_DIR}/${NAME}")
endmacro()

# Remote Repos
macro_FetchContent_Declare(glm #[[    ]] https://github.com/g-truc/glm #[[     ]] 1.0.3 #[[            ]])
macro_FetchContent_Declare(glfw #[[   ]] https://github.com/glfw/glfw #[[      ]] 3.4 #[[              ]])

# GLM
FetchContent_MakeAvailable(glm)

# GLFW
if(EMSCRIPTEN)
  add_library(glfw INTERFACE)
  target_link_options(glfw INTERFACE -sUSE_GLFW=3)
else()
  FetchContent_MakeAvailable(glfw)
endif()

# OpenGL
add_library(opengl INTERFACE)
if(EMSCRIPTEN)
  target_link_options(opengl INTERFACE -sUSE_WEBGL2=1 -sFULL_ES3=1)
elseif(MSVC)
  add_subdirectory(glad)
  target_link_libraries(opengl INTERFACE glad)
else()
  target_link_libraries(opengl INTERFACE GLESv2 EGL)
endif()
