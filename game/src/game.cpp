#include <game/game.hpp>

template <>
auto game::layers::make_layer<game::layers::clear>() -> std::shared_ptr<layer>
{
  return std::make_shared<clear>();
}
template <>
auto game::layers::push_game<game::layers::clear>(engine::application &app) -> void
{
  app.schedule_layer_push<clear>();
}

using namespace game::layers;
using named_layer_map_mapped_t /* */ = struct named_layer_map_mapped_t
{
    decltype(&make_layer /* */<clear>) make_layer = {};
    decltype(&push_game /*  */<clear>) push_game  = {};
};
auto const named_layer_map = []<typename... T>()
{
  return std::unordered_map{std::pair{
      std::string_view{layer_name<T>},
      named_layer_map_mapped_t{
          .make_layer = &make_layer<T>,
          .push_game  = &push_game<T>,
      }}...};
}.operator()<    //
    clear,       //
    boids,       //
    game_of_life //
    >();

auto game::layers::make_layer /* */ (std::string_view name /*                     */) -> std::shared_ptr<layer>
{
  runtime_assert(named_layer_map.contains(name), "unknown layer name {:?}", name);
  auto const make_layer = runtime_assert(named_layer_map.at(name).make_layer, "function nullptr in {:?}:{:?}", "make_layer", name);
  return make_layer();
}
auto game::layers::push_game /*  */ (std::string_view name, engine::application &app) -> void /*             */
{
  runtime_assert(named_layer_map.contains(name), "unknown layer name {:?}", name);
  auto const push_game = runtime_assert(named_layer_map.at(name).push_game, "function nullptr in {:?}:{:?}", "push_game", name);
  return push_game(app);
}
