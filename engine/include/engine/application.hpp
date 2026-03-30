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
    private:
      struct layer_update_schedule_t;

    public:
      auto inline static get() -> application & { return *runtime_assert(s_instance, "null {} access", "application"); }
      using clock /*                       */ = std::chrono::steady_clock;
      using event_container /*             */ = std::any;
      using layer                             = struct layer
      {
          using clock           = application::clock;
          using event_container = application::event_container;
          using update_delay    = std::chrono::duration<double>;
          /**/ virtual ~layer() noexcept {}
          auto virtual on_event(std::any const &event_any) -> void { (void)event_any; }
          auto virtual on_update() -> update_delay { return update_delay::max(); }
          auto virtual on_render() -> void {}

        protected:
          auto inline static app() -> application & { return application::get(); }

        private:
          friend struct application::layer_update_schedule_t;
          clock::time_point m_update_appointment = clock::time_point::min();
      };

    private:
      auto inline static constinit s_instance = static_cast<application *>(nullptr);
      using layer_t                           = layer;
      using layers_mutate_view_t              = struct layers_mutate_view_t
      {
        public:
          using mbr_t             = std::pmr::monotonic_buffer_resource;
          using layers_vec_t      = std::vector<std::shared_ptr<layer_t>>;
          using layers_t          = std::pmr::list<std::shared_ptr<layer_t const>>;
          using layer_locations_t = std::pmr::unordered_map<layer_t const *, layers_t::const_iterator>;

          auto inline layers /*   */ () const noexcept -> layers_t /*    */ const & { return m_layers /*    */; }
          auto inline layer_locations() const noexcept -> layer_locations_t const & { return m_layer_locations; }

          auto inline contains(layer_t const *location) const -> bool { return layer_locations().contains(location); }
          auto inline at /**/ (layer_t const *location) const -> auto { return runtime_assert(layer_locations().contains(location), "layer not found"), layer_locations().at(location); }

          auto inline add(std::shared_ptr<layer_t> layer, layer_t const *location = nullptr) -> void
          {
            runtime_assert<std::logic_error>(layer, "layer was null");
            runtime_assert<std::logic_error>(not layer_locations().contains(layer.get()), "layer duplication");
            auto const where_it  = location ? at(location) : layers().cend();
            auto const insert_it = m_layers /*    */.insert(where_it, std::move(layer));
            /**/ /* */ /*       */ m_layer_locations.insert({insert_it->get(), insert_it});
          }
          auto inline remove(layer_t const *which) -> void
          {
            m_layers /*    */.erase(at(which));
            m_layer_locations.erase(which);
          }

          /**/ inline layers_mutate_view_t(layers_vec_t &layers, size_t expected_max_size, size_t expected_per_item_size = 12zu * sizeof(void *))
              : m_layers_vec /*      */ {&layers},
                m_mbr /*             */ {expected_max_size * expected_per_item_size},
                m_layers /*          */ {&m_mbr},
                m_layer_locations /* */ {&m_mbr}
          {
            m_layer_locations.reserve(expected_max_size);
            for (auto &layer : *m_layers_vec)
            {
              auto const it = m_layers /*    */.insert(m_layers.cend(), std::move(layer));
              /**/ /* */ /**/ m_layer_locations.insert({it->get(), it});
            }
            m_layers_vec->clear();
          }
          /**/ inline ~layers_mutate_view_t()
          {
            auto const nullptr_contamination = m_layer_locations.contains(nullptr);
            if (nullptr_contamination)
            {
              m_layers /*    */.remove(nullptr);
              m_layer_locations.erase(nullptr);
            }
            m_layers_vec->clear();
            for (auto &layer : m_layers) m_layers_vec->push_back(std::const_pointer_cast<layer_t>(std::move(layer)));
            runtime_assert(not nullptr_contamination, "nullptr contamination in layers stack");
          }

          /**/ inline layers_mutate_view_t()                                            = delete;
          /**/ inline layers_mutate_view_t(layers_mutate_view_t const &)                = delete;
          auto inline operator=(layers_mutate_view_t const &) -> layers_mutate_view_t & = delete;

        private:
          layers_vec_t     *m_layers_vec      = {};
          mbr_t             m_mbr             = {};
          layers_t          m_layers          = {};
          layer_locations_t m_layer_locations = {};
      };
      using layers_task_t /*               */ = std::function<void(layers_mutate_view_t &layers)>;
      using layer_update_schedule_t           = struct layer_update_schedule_t
      {
          /** Notes:
           *  Layers should not have duplicate entries in the update schedule (not checked)
           *  Layer appointments are stored inline with layer. Recreating a layer in-place will corrupt the update queue.
           */
        public:
          using layers_t /*                */ = std::vector<std::shared_ptr<layer_t>>;

        private:
          layers_t m_layers = {};
          auto inline heap_op(this auto &&self, auto &op)
          {
            return op(self.m_layers, std::greater{/* min-heap */}, [](layers_t::const_reference layer)
                      { return layer->m_update_appointment; });
          }

        public:
          /**/ inline layer_update_schedule_t() noexcept                                               = default;
          /**/ inline layer_update_schedule_t(layer_update_schedule_t /**/ &&) noexcept                = default;
          /**/ inline layer_update_schedule_t(layer_update_schedule_t const &) /*    */                = default;
          auto inline operator=(layer_update_schedule_t /**/ &&) noexcept -> layer_update_schedule_t & = default;
          auto inline operator=(layer_update_schedule_t const &) /*    */ -> layer_update_schedule_t & = default;

          auto inline is_heap() const noexcept { return heap_op(std::ranges::is_heap); }
          auto inline empty() const noexcept { return m_layers.empty(); }
          auto inline layers() const noexcept { return std::span{m_layers}; }

          auto inline push(std::shared_ptr<layer_t> layer, clock::time_point appointment) -> void
          {
            runtime_assert<std::logic_error>(layer, "null layer");
            layer->m_update_appointment = appointment;
            m_layers.push_back(std::move(layer));
            heap_op(std::ranges::push_heap);
          }
          /// @return [layer, appointment]
          auto inline pop() -> auto
          {
            runtime_assert<std::out_of_range>(not m_layers.empty(), "layers empty");
            heap_op(std::ranges::pop_heap);
            auto layer       = std::move(m_layers.back());
            auto appointment = layer->m_update_appointment;
            m_layers.pop_back();
            return std::tuple{std::move(layer), appointment};
          }
          /// @brief Remove duplicates, nulls, and restores heap ordering
          auto inline restore_heap() -> void
          {
            if (m_layers.size() <= 1zu) return;
            auto       seen    = std::unordered_set<layer const *>{m_layers.size()};
            auto const removed = std::ranges::remove_if(
                m_layers, [&](auto const &layer)
                {
                  auto const keep = layer and not seen.contains(layer.get());
                  if (keep) seen.insert(layer.get());
                  return not keep; //
                });
            m_layers.erase(removed.begin(), removed.end());
            heap_op(std::ranges::make_heap);
          }
          /// @return layers pointer representing the lifetime of mutation (does not own layers vector itself) (restores heap on lifetime end)
          auto inline mutate() -> auto
          {
            auto heap_maintaining_deleter = [this](layers_t *p)
            {
              runtime_assert<std::logic_error>(p == &m_layers, "bad deleter usage");
              restore_heap();
            };
            using mutate_view_t = std::unique_ptr<layers_t, decltype(heap_maintaining_deleter)>;
            return mutate_view_t{&m_layers, std::move(heap_maintaining_deleter)};
          }
      };
      using event_queue_t /*               */ = std::vector<event_container>;

    public:
      template <typename Fn, typename T = std::invoke_result_t<Fn>>
        requires((std::invocable<Fn> and std::same_as<T, std::invoke_result_t<Fn>>) and
                 (std::constructible_from<std::shared_ptr<layer_t>, T> and not std::is_pointer_v<T>))
      auto inline schedule_layer_push(Fn &&make_layer, layer_t const *location = nullptr) -> void
      {
        m_layers_tasks.emplace_back(
            [make_layer = std::forward<Fn>(make_layer), location] //
            (layers_mutate_view_t & layers) mutable
            {
              if (location) (void)layers.at(location); // throws if invalid location
              auto layer = static_cast<std::shared_ptr<layer_t>>(std::invoke(std::move(make_layer)));
              if (layer) layers.add(std::move(layer), location);
            });
      }
      template <typename T, typename... Args>
        requires(std::derived_from<T, layer_t> and std::constructible_from<T, std::remove_cvref_t<Args> && ...>)
      auto inline schedule_layer_push(layer_t const *location = nullptr, Args &&...args) -> void
      {
        return schedule_layer_push(
            [args_tuple = std::tuple{std::forward<Args>(args)...}] mutable
            { return std::apply(
                  [](auto &...args) static
                  { return std::make_shared<T>(std::move(args)...); },
                  args_tuple); },
            location);
      }
      auto inline schedule_layer_push(std::shared_ptr<layer_t> layer, layer_t const *location = nullptr)
      {
        return schedule_layer_push([layer = std::move(layer)] mutable
                                   { return std::move(layer); },
                                   location);
      }
      auto inline schedule_layer_pop(layer_t const *layer) -> void
      {
        m_layers_tasks.emplace_back(
            [layer](layers_mutate_view_t &layers)
            { layers.remove(layer); });
      }

      template <typename T, typename... Args>
        requires(std::constructible_from<T, Args...>)
      auto inline queue_event(Args &&...args) -> void
      {
        m_events.emplace_back(std::in_place_type<T>, std::forward<Args>(args)...);
      }
      template <typename T>
      auto inline queue_event(T &&event) -> void
      {
        return queue_event<std::remove_cvref_t<T>, T>(std::forward<T>(event));
      }

      auto inline get_window /*               */ () const /*    */ -> auto /* */ & { return *runtime_assert(m_window, "null {} access", "main window"); }
      auto inline get_renderer /*             */ () const noexcept -> auto const & { return m_renderer; }
      auto inline get_renderer /*             */ () /* */ noexcept -> auto /* */ & { return m_renderer; }
      auto inline get_layers /*               */ () const noexcept -> auto /*   */ { return std::span{m_layers}; }
      auto inline get_target_render_period /* */ () const noexcept -> auto /*   */ { return /* */ m_target_render_period.count(); }
      auto inline get_target_render_rate /*   */ () const noexcept -> auto /*   */ { return 1.0 / m_target_render_period.count(); }

      auto inline set_target_render_period /* */ (double value) /* */ noexcept -> auto const & { return m_target_render_period = /* */ value * std::chrono::seconds(1); }
      auto inline set_target_render_rate /*   */ (double value) /* */ noexcept -> auto const & { return m_target_render_period = 1.0 / value * std::chrono::seconds(1); }

    public:
      /**/ /*  */ application();
      /**/ /*  */ ~application();
      auto /*  */ run() -> int;

    private:
      GLFWwindow                           *m_window                = {};
      renderer                              m_renderer              = {};

      std::vector<std::shared_ptr<layer_t>> m_layers                = {};
      std::vector<layers_task_t>            m_layers_tasks          = {};

      layer_update_schedule_t               m_layer_update_schedule = {};
      clock::time_point                     m_render_appointment    = clock::now();
      std::chrono::duration<double>         m_target_render_period  = std::chrono::seconds(1) * 1.0 / 60.0;

      event_queue_t                         m_events                = {};
  };
  auto startup(application &app) -> void;
} // namespace engine

#endif // ENGINE_APPLICATION_HPP
