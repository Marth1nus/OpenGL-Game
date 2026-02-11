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
target_include_directories(engine
  PUBLIC include)
target_link_libraries(engine
  PRIVATE enable_warnings
  PUBLIC glm glfw opengl)
target_precompile_headers(engine
  PUBLIC include/engine/core.hpp)

