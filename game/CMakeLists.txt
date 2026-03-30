add_library(game STATIC

  include/game/boids.hpp
  include/game/controls.hpp
  include/game/game_of_life.hpp
  include/game/game.hpp
  include/game/imgui.hpp

  src/boids.cpp
  src/controls.cpp
  src/game_of_life.cpp
  src/game.cpp
  src/imgui.cpp
  src/startup.cpp

)
target_include_directories(game
  PUBLIC include)
target_link_libraries(game
  PRIVATE enable_warnings
  PUBLIC engine imgui)
target_precompile_headers(engine
  PUBLIC include/game/game.hpp)
