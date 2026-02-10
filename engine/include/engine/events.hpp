#include <engine/application.hpp>
#include <engine/core.hpp>
#include <engine/utilities.hpp>

namespace engine::events::glfw
{
  struct key_event
  {
      GLFWwindow *window;
      int         key;
      int         scancode;
      int         action;
      int         mods;
  };
  struct char_event
  {
      GLFWwindow  *window;
      unsigned int codepoint;
  };
  struct drop_event
  {
      GLFWwindow                        *window;
      std::vector<std::filesystem::path> paths;
  };
  struct scroll_event
  {
      GLFWwindow *window;
      glm::dvec2  offset;
  };
  struct char_mods_event
  {
      GLFWwindow  *window;
      unsigned int codepoint;
      int          mods;
  };
  struct cursor_pos_event
  {
      GLFWwindow *window;
      glm::dvec2  pos;
  };
  struct window_pos_event
  {
      GLFWwindow *window;
      glm::ivec2  pos;
  };
  struct window_size_event
  {
      GLFWwindow *window;
      glm::ivec2  size;
  };
  struct cursor_enter_event
  {
      GLFWwindow *window;
      bool        entered;
  };
  struct window_close_event
  {
      GLFWwindow *window;
  };
  struct mouse_button_event
  {
      GLFWwindow *window;
      int         button;
      int         action;
      int         mods;
  };
  struct window_focus_event
  {
      GLFWwindow *window;
      bool        focused;
  };
  struct window_iconify_event
  {
      GLFWwindow *window;
      bool        iconified;
  };
  struct window_refresh_event
  {
      GLFWwindow *window;
  };
  struct window_maximize_event
  {
      GLFWwindow *window;
      bool        maximized;
  };
  struct framebuffer_size_event
  {
      GLFWwindow *window;
      glm::ivec2  size;
  };
  struct window_content_scale_event
  {
      GLFWwindow *window;
      glm::fvec2  scale;
  };
  struct error_event
  {
      int         error_code;
      std::string description;
  };
  struct monitor_event
  {
      GLFWmonitor *monitor;
      int          event;
  };
  struct joystick_event
  {
      int jid;
      int event;
  };
} // namespace engine::events::glfw
namespace engine::events { using namespace glfw; }