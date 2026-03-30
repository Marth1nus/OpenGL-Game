#include <engine/application.hpp>
#include <engine/events.hpp>

#if /* */ defined(INIT_GLAD) or defined(RUN_MAIN_LOOP)
#error "Macro name collision"
#endif // defined(INIT_GLAD) or defined(RUN_MAIN_LOOP)

#if /* */ defined(_WIN32) or defined(_WIN64)
#define INIT_GLAD(load_proc) ::gladLoadGLES2Loader(reinterpret_cast<::GLADloadproc>(load_proc))
#else  // defined(_WIN32) or defined(_WIN64)
#define INIT_GLAD(load_proc) true
#endif // defined(_WIN32) or defined(_WIN64)

#if /* */ defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define RUN_MAIN_LOOP []<typename T>(T &main_loop) static                               \
{                                                                                       \
  auto static constinit s_main_loop_ptr = static_cast<T *>(nullptr);                    \
  /*  */ runtime_assert(s_main_loop_ptr == nullptr, "application singleton violation"); \
  /*                 */ s_main_loop_ptr = &main_loop;                                   \
  ::emscripten_set_main_loop(+[] static { if (not (*s_main_loop_ptr)()) ::emscripten_cancel_main_loop(); }, 0, 1);                                     \
  /*                 */ s_main_loop_ptr = nullptr;                                      \
}
#else  // defined(__EMSCRIPTEN__)
#define RUN_MAIN_LOOP []<typename T>(T &main_loop) static { while (main_loop()); }
#endif // defined(__EMSCRIPTEN__)

/**/ engine::application::application()
{
  runtime_assert(not s_instance, "application singleton violation");
  s_instance = this;
  glfwSetErrorCallback(+[](int error, char const *description)
                       { runtime_assert(false, "GLFW Error 0x{:x}: {}", error, description); });
  runtime_assert(glfwInit(), "{} init fail", "glfw");
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  m_window = glfwCreateWindow(720, 720, "game", nullptr, nullptr); /* TODO: hardcoded parameters */
  runtime_assert(m_window, "{} init fail", "window");
  glfwMakeContextCurrent(m_window);
  runtime_assert(INIT_GLAD(glfwGetProcAddress), "{} init fail", "glad");
  if (auto constexpr attach_glfw_event_callbacks = true;
      attach_glfw_event_callbacks)
  {
    using namespace engine::events::glfw;
    glfwSetKeyCallback(
        m_window,
        +[](GLFWwindow *window, int key, int scancode, int action, int mods)
        { application::get().queue_event(key_event{window, key, scancode, action, mods}); });
    glfwSetCharCallback(
        m_window,
        +[](GLFWwindow *window, unsigned int codepoint)
        { application::get().queue_event(char_event{window, codepoint}); });
    glfwSetDropCallback(
        m_window,
        +[](GLFWwindow *window, int path_count, const char *paths[])
        { application::get().queue_event(drop_event{window, std::vector<std::filesystem::path>{paths, paths + path_count}}); });
    glfwSetScrollCallback(
        m_window,
        +[](GLFWwindow *window, double xoffset, double yoffset)
        { application::get().queue_event(scroll_event{window, {xoffset, yoffset}}); });
    /* TODO: unimplemented in emscripten. disabled for now. */ if (0)
      glfwSetCharModsCallback(
          m_window,
          +[](GLFWwindow *window, unsigned int codepoint, int mods)
          { application::get().queue_event(char_mods_event{window, codepoint, mods}); });
    glfwSetCursorPosCallback(
        m_window,
        +[](GLFWwindow *window, double xpos, double ypos)
        { application::get().queue_event(cursor_pos_event{window, {xpos, ypos}}); });
    glfwSetWindowPosCallback(
        m_window,
        +[](GLFWwindow *window, int xpos, int ypos)
        { application::get().queue_event(window_pos_event{window, {xpos, ypos}}); });
    glfwSetWindowSizeCallback(
        m_window,
        +[](GLFWwindow *window, int width, int height)
        { application::get().queue_event(window_size_event{window, {width, height}}); });
    glfwSetCursorEnterCallback(
        m_window,
        +[](GLFWwindow *window, int entered)
        { application::get().queue_event(cursor_enter_event{window, entered == GLFW_TRUE}); });
    glfwSetWindowCloseCallback(
        m_window,
        +[](GLFWwindow *window)
        { application::get().queue_event(window_close_event{window}); });
    glfwSetMouseButtonCallback(
        m_window,
        +[](GLFWwindow *window, int button, int action, int mods)
        { application::get().queue_event(mouse_button_event{window, button, action, mods}); });
    glfwSetWindowFocusCallback(
        m_window,
        +[](GLFWwindow *window, int focused)
        { application::get().queue_event(window_focus_event{window, focused == GLFW_TRUE}); });
    glfwSetWindowIconifyCallback(
        m_window,
        +[](GLFWwindow *window, int iconified)
        { application::get().queue_event(window_iconify_event{window, iconified == GLFW_TRUE}); });
    glfwSetWindowRefreshCallback(
        m_window,
        +[](GLFWwindow *window)
        { application::get().queue_event(window_refresh_event{window}); });
    glfwSetWindowMaximizeCallback(
        m_window,
        +[](GLFWwindow *window, int maximized)
        { application::get().queue_event(window_maximize_event{window, maximized == GLFW_TRUE}); });
    glfwSetFramebufferSizeCallback(
        m_window,
        +[](GLFWwindow *window, int width, int height)
        { application::get().queue_event(framebuffer_size_event{window, {width, height}}); });
    glfwSetWindowContentScaleCallback(
        m_window,
        +[](GLFWwindow *window, float xscale, float yscale)
        { application::get().queue_event(window_content_scale_event{window, {xscale, yscale}}); });
    glfwSetErrorCallback(
        /* */
        +[](int error_code, char const *description)
        { application::get().queue_event(error_event{error_code, description}); });
    glfwSetMonitorCallback(
        /* */
        +[](GLFWmonitor *monitor, int event)
        { application::get().queue_event(monitor_event{monitor, event}); });
    /* TODO: Figure out why this hangs on windows then remove this line */ if (0)
      glfwSetJoystickCallback(
          /* */
          +[](int jid, int event)
          { application::get().queue_event(joystick_event{jid, event}); });
  }
}
/**/ engine::application::~application()
{
  m_events                = {};
  m_layer_update_schedule = {};
  m_layers_tasks          = {};
  m_layers                = {};
  m_renderer              = {};
  m_window                = (glfwDestroyWindow(m_window), nullptr);
  glfwTerminate();
  runtime_assert(s_instance == this, "application singleton violation");
  s_instance = nullptr;
}
auto engine::application::run() -> int
{
  auto const main_loop = [this] -> bool
  {
    if (glfwWindowShouldClose(m_window)) return false;
    auto const render_dt          = std::chrono::duration_cast<clock::duration>(m_target_render_period);
    auto const render_appointment = std::exchange(m_render_appointment, std::max(m_render_appointment, clock::now()) + render_dt);

    /* layer tasks   */ if (not/*     */ m_layers_tasks.empty()) [[unlikely]]
    {
      m_layer_update_schedule.mutate()->clear();                   // all layers ref count -1
      for (auto   layers_mutate_view = layers_mutate_view_t{m_layers, m_layers.size() + m_layers_tasks.size()};
           auto &&layers_task : std::exchange(m_layers_tasks, {})) // Note `m_layers_tasks` does not maintain capacity
      {
        try
        {
          layers_task(layers_mutate_view);
        }
        catch (std::exception const &e)
        {
          std::println(stderr, "Error in {:?}: {}", "Layer stack manipulations", e.what());
        }
      }
      m_layer_update_schedule.mutate()->assign_range(m_layers); // Note all layers ref count +1
    }
    /* events        */ if (true) /*                          */ [[likely]]
    {
      glfwPollEvents();
      auto const events_size = m_events.size();
      for (auto i = 0zu; i < events_size; ++i)
      {
        auto const &event = m_events.at(i);
        for (auto const &layer : m_layers)
        {
          try
          {
            layer->on_event(event);
          }
          catch (std::exception const &e)
          {
            std::println(stderr, "Error in {:?}: {}", "Layer events", e.what());
          }
        }
      }
      m_events.erase(m_events.begin(), m_events.begin() + events_size);
    }
    /* update layers */ if (not m_layer_update_schedule.empty()) [[likely]]
    {
      while (not m_layer_update_schedule.is_heap())
      {
        std::println("[warning] {}", "update schedule heap corruption");
        m_layer_update_schedule.restore_heap();
      }
      while (not m_layer_update_schedule.empty())
      {
        if (render_appointment < clock::now()) break;
        auto const [layer, appointment] = m_layer_update_schedule.pop();
        if (not layer) continue;
        if (render_appointment < appointment)
        {
          m_layer_update_schedule.push(layer, appointment);
          break;
        }
        if (appointment > clock::now()) std::this_thread::sleep_until(appointment);
        try
        {
          auto const update_delay          = layer->on_update();
          auto const update_delay_duration = std::chrono::duration_cast<clock::duration>(update_delay);
          auto const next_appointment      = std::max(clock::now(), appointment + update_delay_duration);
          m_layer_update_schedule.push(layer, next_appointment);
        }
        catch (std::exception const &e)
        {
          std::println(stderr, "Error in {:?}: {}", "Layer update", e.what());
        }
      }
    }
    /* render layers */ if (true) /*                          */ [[likely]]
    {
      if (render_appointment > clock::now()) std::this_thread::sleep_until(render_appointment);
      if (true) /* viewport fit: center zoom to fit */ [[likely]]
      { /* TODO: this feature is hardcoded consider setting up an enum? */
        auto window_size = glm::i32vec2{};
        glfwGetWindowSize(m_window, &window_size.x, &window_size.y);
        auto const vmax       = std::max(window_size.x, window_size.y);
        auto const window_pos = (window_size - vmax) / 2;
        glViewport(window_pos.x, window_pos.y, vmax, vmax);
      }
      for (auto const &layer : get_layers())
      {
        try /* TODO: consider enforcing `layer::render` to be `noexcept` */
        {
          layer->on_render();
        }
        catch (std::exception const &e)
        {
          std::println(stderr, "Error in {:?}: {}", "Layer render", e.what());
        }
      }
      glfwSwapBuffers(m_window);
    }

    return true;
  };
  RUN_MAIN_LOOP(main_loop); /* equivalent to `while (main_loop());` */
  return EXIT_SUCCESS;
}