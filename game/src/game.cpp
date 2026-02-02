#include <game/game.hpp>

auto game::layers::make_layer(std::string_view name) -> std::shared_ptr<layer>
{
  auto const static layer_makers = std::unordered_map<std::string_view, std::shared_ptr<layer> (*)()>{
      {layer_name<boids /*        */>, &make_layer<boids /*        */>},
      {layer_name<game_of_life /* */>, &make_layer<game_of_life /* */>},
  };
  runtime_assert(layer_makers.contains(name), "game {:?} not found", name);
  return layer_makers.at(name)();
}
auto game::layers::push_game(std::string_view name, engine::application &app) -> void
{
  auto const static game_pushers = std::unordered_map<std::string_view, void (*)(engine::application &)>{
      {game_name<boids /*        */>, &push_game<boids /*        */>},
      {game_name<game_of_life /* */>, &push_game<game_of_life /* */>},
  };
  runtime_assert(game_pushers.contains(name), "game {:?} not found", name);
  game_pushers.at(name)(app);
}
