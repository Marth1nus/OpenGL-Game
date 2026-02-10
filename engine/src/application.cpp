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

engine::application::application()
{
  runtime_assert(not s_instance, "application singleton violation");
  s_instance = this;
  runtime_assert(glfwInit(), "{} init fail", "glfw");
  m_window = glfwCreateWindow(720, 720, "game", nullptr, nullptr); /* TODO: hardcoded parameters */
  runtime_assert(m_window, "{} init fail", "window");
  glfwMakeContextCurrent(m_window);
  runtime_assert(INIT_GLAD(glfwGetProcAddress), "{} init fail", "glad");
  /* glfw event callbacks */ if (true)
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
        +[](int error_code, const char *description)
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
engine::application::~application()
{
  m_layers_tasks          = {};
  m_layer_update_schedule = {};
  m_layers                = {};
  m_renderer              = {};
  m_window                = (glfwDestroyWindow(m_window), nullptr);
  glfwTerminate();
  runtime_assert(s_instance == this, "application singleton violation");
  s_instance = nullptr;
}
auto engine::application::schedule_layer_pop(std::shared_ptr<layer_t const> layer) -> void
{
  schedule_layer_manipulation(
      [layer = std::move(layer)](layers_t &layers) mutable
      {
        auto const it = std::ranges::find(layers, layer);
        runtime_assert(it != layers.end(), "layer {} {} in layer stack", static_cast<void const *>(layer.get()), "not");
        layers.erase(it);
        layer = {}; // layer destruct here
      });
}
auto engine::application::run() -> int
{
  auto const main_loop = [this] -> bool
  {
    if (glfwWindowShouldClose(m_window)) return false;
    auto const render_dt          = std::chrono::duration_cast<clock::duration>(m_target_render_period);
    auto const render_appointment = std::exchange(m_render_appointment, std::max(m_render_appointment, clock::now()) + render_dt);
    /* layer tasks   */ if (not m_layers_tasks.empty())
    {
      for (auto &task : std::exchange(m_layers_tasks, {}))
      {
        try
        {
          task(m_layers);
          if (m_layers.empty()) m_renderer = {}; /* TODO: Reconsider. Could cause issues later on. */
          runtime_assert(m_layers_tasks.empty(), "{0:?} can not be scheduled from a {0:?} task", "between frame layer manipulation");
        }
        catch (std::exception const &e)
        {
          std::println(stderr, "Error in {:?}: {}", "between frame layer manipulation", e.what());
          m_layers_tasks = {};
        };
      }

      if (auto const nulls = std::ranges::remove(m_layers, nullptr);
          not nulls.empty() /* remove nulls in m_layers */)
        m_layers.erase(nulls.begin(), nulls.end());

      auto appointments = std::unordered_map<layer_t const *, clock::time_point>{};
      appointments.reserve(m_layer_update_schedule.size());
      while (not m_layer_update_schedule.empty()) // empty out m_layer_update_schedule
      {
        auto const [time, index, layer] = m_layer_update_schedule.top();
        /*                             */ m_layer_update_schedule.pop();
        if (layer.expired()) continue;
        auto const [it, inserted] /* */ = appointments.insert({layer.lock().get(), time});
        runtime_assert(inserted, "layer {} was found twice in the update schedule", static_cast<void *>(layer.lock().get()));
        (void)index, (void)it;           // unused
      }
      for (auto const &layer : m_layers) // populate m_layer_update_schedule
      {
        auto const index       = static_cast<size_t>(&layer - m_layers.data());
        auto const appointment = appointments.contains(layer.get()) ? appointments.at(layer.get()) : clock::now();
        m_layer_update_schedule.push({appointment, index, layer});
      }
    }
    /* events        */ if (true)
    {
      glfwPollEvents();
      std::swap(m_events, m_events_swap);
      m_events.reserve(m_events_swap.capacity());
      for (auto const &event : m_events_swap)
      {
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
      m_events_swap.clear();
    }
    /* update layers */ while (not m_layer_update_schedule.empty())
    {
      auto const [appointment, index, layer] = m_layer_update_schedule.top();
      if (render_appointment < appointment or
          render_appointment < clock::now()) break;
      m_layer_update_schedule.pop();
      if (layer.expired()) continue;
      std::this_thread::sleep_until(appointment);
      try
      {
        auto const update_delay          = layer.lock()->on_update();
        auto const update_delay_duration = std::chrono::duration_cast<clock::duration>(update_delay);
        auto const next_appointment      = std::max(clock::now(), appointment + update_delay_duration);
        m_layer_update_schedule.push({next_appointment, index, layer});
      }
      catch (std::exception const &e)
      {
        std::println(stderr, "Error in {:?}: {}", "Layer update", e.what());
      }
    }
    /* render layers */ if (true)
    {
      /* viewport fit: center zoom to fit */ { /* TODO: this feature is hardcoded consider setting up an enum? */
        int window_width, window_height;
        glfwGetWindowSize(m_window, &window_width, &window_height);
        auto const vmax     = std::max(window_width, window_height);
        auto const window_x = (window_width - vmax) / 2;
        auto const window_y = (window_height - vmax) / 2;
        glViewport(window_x, window_y, vmax, vmax);
      }
      std::this_thread::sleep_until(render_appointment);
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