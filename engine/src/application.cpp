#include <engine/application.hpp>

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
#define RUN_MAIN_LOOP []<typename T>(T &main_loop) static            \
{                                                                    \
  auto static constinit s_main_loop_ptr = static_cast<T *>(nullptr); \
  /*                 */ s_main_loop_ptr = &main_loop;                \
  ::emscripten_set_main_loop(+[] static { if (not (*s_main_loop_ptr)()) ::emscripten_cancel_main_loop(); }, 0, 1);                  \
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
    auto const render_rate        = 60.0 /* TODO: hardcoded 60fps */;
    auto const render_dt          = std::chrono::duration_cast<clock::duration>(std::chrono::seconds(1) / render_rate);
    auto const render_appointment = clock::now() + render_dt;
    /* layer tasks   */ if (not m_layers_tasks.empty())
    {
      for (auto &task : std::exchange(m_layers_tasks, {}))
      {
        try
        {
          task(m_layers);
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
        auto const update_delay          = layer.lock()->update();
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
      std::this_thread::sleep_until(render_appointment);
      for (auto const &layer : get_layers())
      {
        try /* TODO: consider enforcing `layer::render` to be `noexcept` */
        {
          layer->render();
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
  RUN_MAIN_LOOP(main_loop); // equivalent to `while (main_loop());`
  return EXIT_SUCCESS;
}