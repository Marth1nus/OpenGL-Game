#include <game/game.hpp>

auto engine::startup(application &app) -> void
{
  game::layers::push_game("boids", app);
}