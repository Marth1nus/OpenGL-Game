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
  
  template <typename T>
  auto inline constexpr layer_name_unchecked = static_cast<char const *>(nullptr);
#define TEMPLATE_SPECIALIZE(layer_type) \
  template <>                           \
  auto inline constexpr layer_name_unchecked<layer_type> = #layer_type
  TEMPLATE_SPECIALIZE(clear);
  TEMPLATE_SPECIALIZE(boids);
  TEMPLATE_SPECIALIZE(game_of_life);
#undef TEMPLATE_SPECIALIZE
  template <typename T>
  concept named_layer = layer_name_unchecked<T> != nullptr;
  template <named_layer T>
  auto inline constexpr layer_name = layer_name_unchecked<T>;
  template <named_layer T>
  auto make_layer(/*                 */ /*                     */ /*                       */) -> std::shared_ptr<layer>;
  auto make_layer(std::string_view name /*                     */ /*                       */) -> std::shared_ptr<layer>;
  template <named_layer T>
  auto push_game(/*                  */ engine::application &app = engine::application::get()) -> void;
  auto push_game(std::string_view name, engine::application &app = engine::application::get()) -> void;

} // namespace game::layers
struct game::layers::clear : layer
{
    glm::vec4 color = {0.1f, 0.1f, 0.1f, 1.0f};
    /**/ clear(glm::vec4 color) noexcept : color{color} {}
    /**/ clear() noexcept = default;
    auto render() -> void override
    {
      glClearColor(color.r, color.g, color.b, color.a);
      glClear(GL_COLOR_BUFFER_BIT);
    }
};

#endif // GAME_HPP
