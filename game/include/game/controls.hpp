#ifndef GAME_CONTROLS_HPP
#define GAME_CONTROLS_HPP

#include <game/game.hpp>
#include <game/imgui.hpp>

namespace game::layers { struct controls; }
struct game::layers::controls : game::imgui::host
{
    /**/ controls(std::string_view game_name = "none");
    /**/ ~controls();
    auto switch_game(std::string_view game_name) -> void;
    auto get_game() const -> /* [name, index, is_switching] */ std::tuple<std::string_view, size_t, bool>;
    auto on_update() -> update_delay override;
    auto on_imgui() -> void override;

  private:
    std::vector<char const *>           m_games              = {"none", "clear", "boids", "game of life"};
    std::make_signed_t<size_t>          m_current_game_index = 0z;
    std::vector<std::shared_ptr<layer>> m_layers             = {};
};

#endif // GAME_CONTROLS_HPP