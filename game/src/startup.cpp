#include <game/game.hpp>

auto engine::startup(application &app) -> void
{
  game::layers::push_game("game_of_life", app);
  glfwSetKeyCallback(
      &app.get_window(), /* TODO: Implement Menu Layer */
      +[](GLFWwindow *window, int key, int scancode, int action, int mods) static
      {
        (void)window, (void)scancode; /* unused */
        auto const next_game = [&] -> std::string_view
        {
          if ((mods & GLFW_MOD_ALT) == 0 or
              (action != GLFW_PRESS)) return {};
          switch (key)
          {
            case GLFW_KEY_1: return "boids";
            case GLFW_KEY_2: return "game_of_life";
            default /*   */: return {};
          }
        }();
        if (not next_game.empty())
        {
          try
          {
            auto &app = engine::application::get();
            app.schedule_layer_manipulation(&engine::application::layers_t::clear);
            game::layers::push_game(next_game, app);
          }
          catch (std::exception const &e)
          {
            std::println(stderr, "Error in {:?}: {}", "Temporary Key Callback", e.what());
          }
        }
      });
}