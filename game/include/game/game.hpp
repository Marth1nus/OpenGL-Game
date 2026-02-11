#ifndef GAME_HPP
#define GAME_HPP

#include <engine/application.hpp>

#if /* */ defined(TEMPLATE_SPECIALIZE)
#error "Macro name collision"
#endif // defined(TEMPLATE_SPECIALIZE)

namespace game
{
  namespace utilities = engine::utilities;
  using utilities::runtime_assert;
} // namespace game
namespace game::layers
{
  using engine::application;
  using layer = application::layer;

  struct clear;
  struct boids;
  struct game_of_life;
  struct keyboard_control_later;

  /* used for `for named layer` operations */
  using named_layer_ptr_variant_t = std::variant< //
      clear *,                                    //
      boids *,                                    //
      game_of_life *,                             //
      keyboard_control_later *>;

  template <typename T>
  auto inline constexpr layer_name_unchecked = static_cast<char const *>(nullptr);
#define TEMPLATE_SPECIALIZE(layer_type) \
  template <>                           \
  auto inline constexpr layer_name_unchecked<layer_type> = #layer_type
  TEMPLATE_SPECIALIZE(clear);
  TEMPLATE_SPECIALIZE(boids);
  TEMPLATE_SPECIALIZE(game_of_life);
  TEMPLATE_SPECIALIZE(keyboard_control_later);
#undef TEMPLATE_SPECIALIZE
  template <typename T>
  concept named_layer = layer_name_unchecked<T> != nullptr;
  template <named_layer T>
  auto inline constexpr layer_name = layer_name_unchecked<T>;
  template <named_layer T>
  extern auto push_layer(/*                  */ bool game_layers = false, engine::application &app = engine::application::get()) -> void;
  extern auto push_layer(std::string_view name, bool game_layers = false, engine::application &app = engine::application::get()) -> void;
  using named_layer_map_t = std::unordered_map<std::string_view, decltype(&push_layer<clear>)>;
  extern const named_layer_map_t named_layer_map;

} // namespace game::layers

struct game::layers::clear : layer
{
    glm::vec4 color = {0.1f, 0.1f, 0.1f, 1.0f};
    /**/ clear(glm::vec4 color) noexcept : color{color} {}
    /**/ clear() noexcept = default;
    auto on_render() -> void override
    {
      glClearColor(color.r, color.g, color.b, color.a);
      glClear(GL_COLOR_BUFFER_BIT);
    }
};
template <>
auto inline game::layers::push_layer<game::layers::clear>(bool game_layers, engine::application &app) -> void
{
  (void)game_layers; /* unused */
  app.schedule_layer_push<clear>();
}
#endif // GAME_HPP
