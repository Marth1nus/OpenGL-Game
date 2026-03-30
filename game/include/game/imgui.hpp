#ifndef GAME_IMGUI_HPP
#define GAME_IMGUI_HPP

#include <game/game.hpp>
#include <imgui.h>

namespace game::imgui
{
  struct imgui_lifetime;
  struct host;
  struct client
  {
    public:
      /**/ virtual ~client() {}
      auto virtual on_imgui() -> void = 0;

    private:
      friend struct host;
  };
  struct host : layer, client
  {
    private:
      std::shared_ptr<imgui_lifetime> m_imgui_lifetime = {};

    public:
      std::vector<std::shared_ptr<client>> clients = {};

    public:
      /**/ host(std::vector<std::shared_ptr<client>> clients = {});
      /**/ ~host();
      auto on_render() -> void override;
      auto on_imgui() -> void override;
  };
} // namespace game::imgui
namespace game::layers { using imgui_host = game::imgui::host; }

#endif // GAME_IMGUI_HPP