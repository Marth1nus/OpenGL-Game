add_library(game STATIC

  include/game/game.hpp

  src/boids.cpp
  src/game_of_life.cpp
  src/game.cpp
  src/startup.cpp

)
target_include_directories(game
  PUBLIC include)
target_link_libraries(game
  PRIVATE enable_warnings
  PUBLIC engine)
