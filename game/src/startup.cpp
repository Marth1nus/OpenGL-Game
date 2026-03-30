#include <game/controls.hpp>
#include <game/game.hpp>

auto engine::startup(application &app) -> void
{
  app.schedule_layer_push<game::layers::controls>(nullptr, "boids");
}