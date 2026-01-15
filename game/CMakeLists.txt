add_library(game STATIC

  include/game/game.hpp

  src/game.cpp

)
target_include_directories(game PUBLIC include)
target_link_libraries(game
  PRIVATE enable_warnings
  PUBLIC engine)
