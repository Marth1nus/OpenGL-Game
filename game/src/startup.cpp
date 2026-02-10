#include <engine/events.hpp>
#include <game/game.hpp>

struct temp_control_layer : engine::application::layer
{
    auto on_event(std::any const &event_any) -> void override
    {
      if (auto const event_ptr = std::any_cast<engine::events::key_event>(&event_any))
      {
        auto const &event    = *event_ptr,
                   &ev       = event;
        auto const next_game = [&] -> std::string_view
        {
          if ((ev.mods & GLFW_MOD_ALT) == 0 or
              (ev.action != GLFW_PRESS)) return {};
          switch (ev.key)
          {
            case GLFW_KEY_1: return "boids";
            case GLFW_KEY_2: return "game_of_life";
            default /*   */: return {};
          }
        }();
        if (not next_game.empty())
        {
          auto &app = engine::application::get();
          app.schedule_layer_manipulation(&engine::application::layers_t::clear);
          app.schedule_layer_push<temp_control_layer>();
          game::layers::push_game(next_game, app);
        }
      }
    }
};

auto engine::startup(application &app) -> void
{
  app.schedule_layer_push<temp_control_layer>();
  game::layers::push_game("boids", app);
}