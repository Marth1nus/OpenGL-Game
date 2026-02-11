#include <game/game.hpp>

auto engine::startup(application &app) -> void
{
  game::layers::push_layer("keyboard_control_later", true, app);
}