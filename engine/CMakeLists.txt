add_library(engine STATIC

  include/engine/application.hpp
  include/engine/core.hpp
  include/engine/renderer.hpp
  include/engine/utilities.hpp

  src/application.cpp
  src/core.cpp
  src/renderer.cpp
  src/utilities.cpp

)
target_include_directories(engine PUBLIC include)
target_link_libraries(engine
  PRIVATE enable_warnings
  PUBLIC glm::glm)

# Link GLFW
if(EMSCRIPTEN)
  target_link_options(engine PUBLIC -sUSE_GLFW=3)
else()
  target_link_libraries(engine PUBLIC glfw)
endif()

# Link OpenGL ES 3.2
if(EMSCRIPTEN)
  target_link_options(engine PUBLIC -sUSE_WEBGL2=1 -sFULL_ES3=1)
elseif(MSVC)
  target_link_libraries(engine PUBLIC glad)
else()
  target_link_libraries(engine PUBLIC GLESv2 EGL)
endif()
