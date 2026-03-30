#ifndef GAME_HPP
#define GAME_HPP

#include <engine/engine.hpp>

namespace game
{
  namespace utilities = engine::utilities;
  namespace events    = engine::events;
  using layer         = engine::application::layer;
  using utilities::runtime_assert;
} // namespace game
namespace game::layers { struct clear; }
struct game::layers::clear : layer
{
    glm::vec4 color       = {0.1f, 0.1f, 0.1f, 1.0f};
    /**/ clear() noexcept = default;
    /**/ clear(glm::vec4 color) noexcept : color{color} {}
    auto on_render() -> void override
    {
      glClearColor(color.r, color.g, color.b, color.a);
      glClear(GL_COLOR_BUFFER_BIT);
    }
};
#endif // GAME_HPP
