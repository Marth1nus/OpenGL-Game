#ifndef ENGINE_APPLICATION_HPP
#define ENGINE_APPLICATION_HPP

#include <engine/core.hpp>
#include <engine/renderer.hpp>
#include <engine/utilities.hpp>

#include <GLFW/glfw3.h>

namespace engine
{
  struct application
  {
    public:
      auto inline static get() -> application & { return *runtime_assert(s_instance, "null {} access", "application"); }
      using clock = std::chrono::steady_clock;
      struct layer
      {
          using update_delay = std::chrono::duration<double>;
          /**/ virtual ~layer() noexcept {}
          auto virtual update() -> update_delay = 0;
          auto virtual render() -> void         = 0;

        protected:
          auto inline static app() -> application & { return application::get(); }
      };

    private:
      auto inline static constinit s_instance = static_cast<application *>(nullptr);
      using layer_t                           = layer;
      using layers_t                          = std::vector<std::shared_ptr<layer_t>>;
      using layers_task_t                     = std::function<void(layers_t &layers)>;
      using clock_t                           = clock;
      using layer_update_appointment_t        = struct layer_update_appointment
      {
          clock_t::time_point    appointment                      = {};
          size_t                 render_index_for_stable_ordering = {};
          std::weak_ptr<layer_t> layer                            = {};
          auto inline constexpr priority() const noexcept { return std::tuple{appointment, render_index_for_stable_ordering}; }
          auto inline constexpr operator<=>(layer_update_appointment const &o) const noexcept { return priority() <=> o.priority(); }
      };
      using layer_update_schedule_t = std::priority_queue<layer_update_appointment_t,
                                                          std::vector<layer_update_appointment_t>,
                                                          std::greater<layer_update_appointment_t>>;

    public:
      /**/ /*  */ application();
      /**/ /*  */ ~application();

      template <typename T>
        requires(std::convertible_to<T, layers_task_t>)
      auto inline schedule_layer_manipulation(T &&task) -> void
      {
        m_layers_tasks.emplace_back(std::forward<T>(task));
      }
      template <typename T>
        requires(std::invocable<T> and std::convertible_to<std::invoke_result_t<T>, std::shared_ptr<layer_t>>)
      auto inline schedule_layer_push(T &&make_layer, std::ptrdiff_t index = -1) -> void
      {
        schedule_layer_manipulation(
            [make_layer = std::forward<T>(make_layer), index](layers_t &layers) mutable
            {
              auto layer = static_cast<std::shared_ptr<layer_t>>(std::invoke(make_layer));
              runtime_assert(std::ranges::find(layers, layer) == layers.end(), "layer {} {} in layer stack", static_cast<void const *>(layer.get()), "already");
              auto const layers_size = static_cast<ptrdiff_t>(layers.size());
              auto const i           = index < 0 ? index + layers_size + 1 : index;
              runtime_assert(0 <= i and i <= layers_size, "out of range");
              layers.emplace(layers.begin() + i, std::move(layer));
            });
      }
      template <typename T, typename... Args>
        requires(std::derived_from<T, layer_t> and std::constructible_from<T, std::remove_cvref_t<Args> && ...>)
      auto inline schedule_layer_push(std::ptrdiff_t index = -1, Args &&...args) -> void
      {
        return schedule_layer_push([args_tuple = std::tuple{std::forward<Args>(args)...}]() mutable
                                   { return std::apply([](auto &...args) static
                                                       { return std::make_shared<T>(std::move(args)...); },
                                                       args_tuple); },
                                   index);
      }
      auto /*  */ schedule_layer_pop(std::shared_ptr<layer_t const> layer) -> void;

      auto inline get_window /*   */ () const /*    */ -> auto /* */ & { return *runtime_assert(m_window, "null {} access", "main window"); }
      auto inline get_renderer /* */ () const noexcept -> auto /* */ & { return m_renderer; }
      auto inline get_renderer /* */ () /* */ noexcept -> auto /* */ & { return m_renderer; }
      auto inline get_layers /*   */ () const noexcept -> auto /*   */ { return std::span{m_layers}; }

      auto /*  */ run() -> int;

    private:
      GLFWwindow                *m_window{};
      renderer                   m_renderer{};
      layers_t                   m_layers{};
      layer_update_schedule_t    m_layer_update_schedule{};
      std::vector<layers_task_t> m_layers_tasks{};
  };
  auto startup(application &app) -> void;
} // namespace engine

#endif // ENGINE_APPLICATION_HPP
