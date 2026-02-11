#include <engine/events.hpp>
#include <game/game.hpp>

static_assert([]<typename... T>(std::variant<T *...>)
              { return (game::layers::named_layer<T> and ...); }(game::layers::named_layer_ptr_variant_t{}),
              "missing layer name in <game/game.hpp>");

struct game::layers::keyboard_control_later : engine::application::layer
{
    auto inline static constexpr layers = []<typename... T>(std::variant<T *...>)
    { return std::array{layer_name<T>...}; }(game::layers::named_layer_ptr_variant_t{});
    auto on_event(std::any const &event_any) -> void override
    {
      auto const event_ptr = std::any_cast<engine::events::key_event>(&event_any);
      if (not event_ptr) return;
      auto const &event = *event_ptr;
      if (not((event.mods & GLFW_MOD_ALT) and
              (event.action == GLFW_PRESS))) return;
      static_assert(GLFW_KEY_9 - GLFW_KEY_0 == 9);
      auto const next_game_i     = event.key - GLFW_KEY_0;
      auto const next_game_i_max = std::min(9, static_cast<int>(layers.size()) - 1);
      if (not(0 <= next_game_i and next_game_i <= next_game_i_max)) return;
      app().schedule_layer_manipulation(&engine::application::layers_t::clear);
      push_layer("keyboard_control_later", false, app());
      push_layer(layers.at(next_game_i), true, app());
    }
};
template <>
auto game::layers::push_layer<game::layers::keyboard_control_later>(bool game_layers, engine::application &app) -> void
{
  app.schedule_layer_push<keyboard_control_later>();
  if (not game_layers) return;
  push_layer(keyboard_control_later::layers.at(1), true, app);
}

const game::layers::named_layer_map_t game::layers::named_layer_map = []<typename... T>(std::variant<T *...> &&)
{ return std::unordered_map{std::pair{std::string_view{layer_name<T>}, &push_layer<T>}...}; }(game::layers::named_layer_ptr_variant_t{});
auto game::layers::push_layer(std::string_view name, bool game_layers, engine::application &app) -> void
{
  runtime_assert(named_layer_map.contains(name), "unknown layer name {:?}", name);
  return named_layer_map.at(name)(game_layers, app);
}
