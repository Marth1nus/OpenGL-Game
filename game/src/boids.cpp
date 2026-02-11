#include <game/game.hpp>

struct game::layers::boids : layer
{
  public:
    struct simulation_settings
    {
        float min_position      = -1.0f,
              max_position      = +1.0f,
              min_velocity      = +0.1f,
              max_velocity      = +0.5f,
              min_acceleration  = +0.1f,
              max_acceleration  = +2.0f;
        float view_distance     = +0.1f,
              boid_width        = view_distance / 4.0f;
        float weight_separation = +0.02f,
              weight_alignment  = +1.2f,
              weight_cohesion   = +1.3f,
              weight_mouse_flee = +100.0f;
        size_t tick_rate        = 60zu,
               boid_count       = 800zu,
               max_neighbors    = 16zu;
        auto inline subspace_width() const noexcept { return view_distance; }
        auto inline subspace_count() const noexcept { return static_cast<glm::i32>(glm::ceil((max_position - min_position) / subspace_width())); }

      public:
        auto /*  */ validate() const -> void
        {
          auto static constexpr verify_sorted = [](std::string_view const name, auto const &values) static
          { return runtime_assert(std::ranges::is_sorted(values), "invalid {:?}", name); };
          verify_sorted("position limits" /*     */, std::array{-1.0f /*    */, min_position /*      */, max_position /*      */, +001.0f});
          verify_sorted("velocity limits" /*     */, std::array{+0.0f /*    */, min_velocity /*      */, max_velocity /*      */, +100.0f});
          verify_sorted("acceleration limits" /* */, std::array{+0.0f /*    */, min_acceleration /*  */, max_acceleration /*  */, +010.0f});
          verify_sorted("view distance" /*       */, std::array{+0.0f /*    */, view_distance /*     */, max_position /*      */});
          verify_sorted("boid width" /*          */, std::array{+0.0f /*    */, boid_width /*        */, view_distance /*     */});
          verify_sorted("weight separation" /*   */, std::array{+0.0001f /* */, weight_separation /* */, +10.0f /*            */});
          verify_sorted("weight alignment" /*    */, std::array{+0.0001f /* */, weight_alignment /*  */, +10.0f /*            */});
          verify_sorted("weight cohesion" /*     */, std::array{+0.0001f /* */, weight_cohesion /*   */, +10.0f /*            */});
          verify_sorted("tick rate" /*           */, std::array{1zu /*      */, tick_rate /*         */, 60zu /*              */});
          verify_sorted("boid count" /*          */, std::array{1zu /*      */, boid_count /*        */, 9'999zu /*           */});
          verify_sorted("max neighbors" /*       */, std::array{1zu /*      */, max_neighbors /*     */, boid_count /*        */});
        }
    };
    struct opengl_handles
    {
        uint32_t pid{}, vid{}, fid{}, vbo{}, vao{};
    };
    struct uniform_locations
    {
        int32_t boid_width{};
    };
    struct statistics
    {
        std::chrono::steady_clock::time_point
               frame_start             = std::chrono::steady_clock::now();
        double average_neighbors       = 0.0,
               average_cycle_duration  = 0.0,
               average_update_duration = 0.0;
        size_t max_neighbors           = 0zu;
    };
    struct boid
    {
        uint32_t              id{}, padding{};
        glm::vec2             position{}, velocity{}, acceleration{};
        auto inline constexpr operator<=>(boid const &o) const noexcept -> auto { return id <=> o.id; }
    };

  public:
    /**/ boids() : boids(simulation_settings{}) {}
    /**/ boids(simulation_settings const &settings) : m_settings{(settings.validate(), settings)}
    {
      m_opengl.pid = glCreateProgram();
      m_opengl.vid = glCreateShader(GL_VERTEX_SHADER);
      m_opengl.fid = glCreateShader(GL_FRAGMENT_SHADER);
      m_opengl.vbo = app().get_renderer().buffers /*      */.activate();
      m_opengl.vao = app().get_renderer().vertexarrays /* */.activate();
      setup();
    }
    /**/ ~boids() override
    {
      m_opengl.pid = (glDeleteProgram(m_opengl.pid), 0u);
      m_opengl.vid = (glDeleteShader(m_opengl.vid), 0u);
      m_opengl.fid = (glDeleteShader(m_opengl.fid), 0u);
      m_opengl.vbo = (app().get_renderer().buffers /*      */.deactivate(m_opengl.vbo), 0u);
      m_opengl.vao = (app().get_renderer().vertexarrays /* */.deactivate(m_opengl.vao), 0u);
    }

  private:
    auto inline static thread_local random = [rd = std::random_device{}]<std::floating_point T = float>(T min = -1.0f, T max = 1.0f) mutable
    { return std::uniform_real_distribution{min, max}(rd); };
    auto setup() -> void
    {
      glBindBuffer(GL_ARRAY_BUFFER, m_opengl.vbo);
      glBufferData(GL_ARRAY_BUFFER, /* size */ 1, nullptr, GL_STATIC_DRAW); // temp buffer data
      glCheckError();

      glBindVertexArray(m_opengl.vao);
      for (auto attrib_index = 0; auto const member : {&boid::position, &boid::velocity, &boid::acceleration})
      {
        auto const offset = &(((boid const *)0)->*member);
        glVertexAttribPointer(attrib_index, (GLint)offset->length(), GL_FLOAT, GL_FALSE, (GLsizei)sizeof(boid), offset);
        glVertexAttribDivisor(attrib_index, 1);
        glEnableVertexAttribArray(attrib_index++);
        glCheckError();
      };

      app().get_renderer().compile_shader /* */ (m_opengl.vid, std::array{m_glsl_version, m_glsl_vertex});
      app().get_renderer().compile_shader /* */ (m_opengl.fid, std::array{m_glsl_version, m_glsl_fragment});
      app().get_renderer().link_program /*   */ (m_opengl.pid, std::array{m_opengl.vid, m_opengl.fid});

      glUseProgram(m_opengl.pid);
      glUniform1f(m_uniforms.boid_width = glGetUniformLocation(m_opengl.pid, "boid_width"), m_settings.boid_width);
      glCheckError();

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glCheckError();

      m_vbo_bytes_size             = {};
      m_tick                       = {};
      m_render_tick                = {};
      m_boids                      = {};
      m_subspaces_allocation_cache = {};
      m_neighbors_allocation_cache = {};
      m_statistics                 = {};
    }

  public:
    auto on_update() -> update_delay override
    {
      auto const update_start     = std::chrono::steady_clock::now();
      auto const dt               = 1.0f / m_settings.tick_rate;
      auto       total_neighbors  = 0zu;
      auto       max_neighbors    = 0zu;
      auto      &subspaces        = m_subspaces_allocation_cache;
      auto const subspace_offsets = std::views::iota(-1, 1 + 1);
      auto const subspaces_count  = glm::vec2{static_cast<float>(m_settings.subspace_count())};
      auto const get_subspace_id  = [subspaces_count](boid const &b) -> subspace_id
      { return {b.position * subspaces_count}; };
      auto const mouse_pos = [&]
      {
        int    window_width, window_height;
        double mouse_x, mouse_y;
        glfwGetWindowSize(&app().get_window(), &window_width, &window_height);
        glfwGetCursorPos(&app().get_window(), &mouse_x, &mouse_y);
        auto const vmax      = std::max(window_width, window_height);
        auto const window_x  = (window_width - vmax) / 2;
        auto const window_y  = (window_height - vmax) / 2;
        auto const mouse_pos = glm::vec2{/* */ (mouse_x - window_x) / vmax * 2.0 - 1.0,
                                         1.0 - (mouse_y - window_y) / vmax * 2.0 /* */};
        return mouse_pos;
      }();
      auto const n_boids = [&]
      {
        if (m_boids.size() == m_settings.boid_count) return m_boids.size();
        auto const old_boid_count = m_boids.size();
        m_boids.resize(m_settings.boid_count);
        for (auto &boid : m_boids | std::views::drop(old_boid_count))
          boid = {
              .id           = static_cast<decltype(boid.id)>(&boid - m_boids.data()),
              .position     = glm::vec2{random(), random()} * m_settings.max_position,
              .velocity     = glm::vec2{random(), random()} * m_settings.min_velocity,
              .acceleration = glm::vec2{random(), random()} * m_settings.min_acceleration,
          };
        return m_boids.size();
      }();
      for (auto &[id, boids] : subspaces) boids.clear();
      for (auto &boid : m_boids) subspaces[get_subspace_id(boid)].push_back(boid);
      for (auto &boid : m_boids)
      {
        auto static constexpr clamp_length = [](glm::vec2 value, float min, float max) -> glm::vec2
        {
          auto const len = glm::length(value);
          if (len < min) value *= min / len;
          if (len > max) value *= max / len;
          return value;
        };
        auto const &neighbors = [&] -> auto &
        {
          auto const subspace_id = get_subspace_id(boid);
          auto      &neighbors   = m_neighbors_allocation_cache;
          neighbors.clear();
          for (auto const y : subspace_offsets)
            for (auto const x : subspace_offsets)
              for (auto const skip_self_check = not(x == 0 and y == 0);
                   auto const b : subspaces[subspace_id + glm::i32vec2{x, y}])
                /**/ if (not skip_self_check and boid.id == b.id)
                  continue;
                else if (auto const distance = glm::distance(boid.position, b.position);
                         distance <= m_settings.view_distance + m_settings.boid_width * 0.5f)
                  neighbors.push_back({b, distance});
          auto static constexpr get_distance_from_pair = &std::remove_reference_t<decltype(neighbors.front())>::second;
          /**/ if (auto const is_neighbor_drop_needed /*               */ = neighbors.size() <= m_settings.max_neighbors)
          {
            (void)is_neighbor_drop_needed;
            /* goto return statement */;
          }
          else if (auto const is_sort_out_greater_distances_cheaper /* */ = neighbors.size() <= m_settings.max_neighbors * 2)
          {
            (void)is_sort_out_greater_distances_cheaper;
            auto const sort_size          = neighbors.size() - m_settings.max_neighbors;
            auto const sort_comp          = std::ranges::less{};
            auto const [unsorted, sorted] = utilities::heap_sort_partial(std::span{neighbors}, sort_size, sort_comp, get_distance_from_pair);
            (void)unsorted, (void)sorted; /* unsorted is already in place */
            neighbors.resize(m_settings.max_neighbors);
          }
          else if (auto const is_sort_in_lesser_distances_cheaper /*   */ = true)
          {
            (void)is_sort_in_lesser_distances_cheaper;
            auto const sort_size          = m_settings.max_neighbors;
            auto const sort_comp          = std::ranges::greater{};
            auto const [unsorted, sorted] = utilities::heap_sort_partial(std::span{neighbors}, sort_size, sort_comp, get_distance_from_pair);
            std::ranges::copy(sorted, neighbors.data()), (void)unsorted;
            neighbors.resize(m_settings.max_neighbors);
          }
          return neighbors;
        }();
        auto const [separation, alignment, cohesion] = [&]
        {
          if (neighbors.size() == 0zu) return std::tuple{glm::vec2{}, glm::vec2{}, glm::vec2{}};
          auto total_separation = glm::vec2{},
               total_velocity   = glm::vec2{},
               total_position   = glm::vec2{};
          for (auto const &[b, distance] : neighbors)
          {
            total_separation += (boid.position - b.position) / (distance * distance * distance);
            total_velocity   += b.velocity;
            total_position   += b.position;
          }
          auto const average_separation = total_separation / static_cast<float>(neighbors.size()),
                     average_velocity   = total_velocity / static_cast<float>(neighbors.size()),
                     average_position   = total_position / static_cast<float>(neighbors.size());
          auto const separation         = clamp_length(average_separation /*         */, 0.0f, +m_settings.max_acceleration),
                     alignment          = clamp_length(average_velocity - boid.velocity, 0.0f, +m_settings.max_acceleration),
                     cohesion           = clamp_length(average_position - boid.position, 0.0f, +m_settings.max_acceleration);
          return std::tuple{separation, alignment, cohesion};
        }();
        auto const [mouse_flee] = [&]
        {
          auto mouse_gap  = boid.position - mouse_pos;
          auto mouse_flee = glm::vec2{};
          if (glm::length(mouse_gap) < 0.1f) mouse_flee = glm::normalize(mouse_gap) * 10.0f;
          return std::tuple{mouse_flee};
        }();
        total_neighbors   += neighbors.size();
        max_neighbors      = std::max(max_neighbors, neighbors.size());
        boid.acceleration += m_settings.weight_separation /* */ * separation /* */ +
                             m_settings.weight_alignment /*  */ * alignment /*  */ +
                             m_settings.weight_cohesion /*   */ * cohesion /*   */ +
                             m_settings.weight_mouse_flee /* */ * mouse_flee /* */;
        boid.acceleration   = clamp_length(boid.acceleration, m_settings.min_acceleration, m_settings.max_acceleration);
        boid.velocity      += boid.acceleration * dt;
        boid.velocity       = clamp_length(boid.velocity, m_settings.min_velocity, m_settings.max_velocity);
        boid.position      += boid.velocity * dt;
        auto const clamped  = glm::clamp(boid.position, m_settings.min_position - m_settings.boid_width, m_settings.max_position + m_settings.boid_width);
        if (boid.position.x != clamped.x) boid.position.x = -clamped.x;
        if (boid.position.y != clamped.y) boid.position.y = -clamped.y;
      }
      total_neighbors                      /= 2zu; // remove double counted connections
      auto const average_neighbors          = static_cast<double>(total_neighbors / n_boids);
      auto const update_end                 = std::chrono::steady_clock::now();
      auto const cycle_end                  = update_end;
      auto const cycle_start                = std::exchange(m_statistics.frame_start, cycle_end);
      auto const update_duration            = std::chrono::duration_cast<std::chrono::duration<double>>(update_end /* */ - update_start /* */).count();
      auto const cycle_duration             = std::chrono::duration_cast<std::chrono::duration<double>>(cycle_end /*  */ - cycle_start /*  */).count();
      m_statistics.max_neighbors            = std::max(m_statistics.max_neighbors, max_neighbors);
      m_statistics.average_neighbors        = (m_statistics.average_neighbors /*       */ * 99.0 + 1.0 * average_neighbors /*  */) / 100.0;
      m_statistics.average_update_duration  = (m_statistics.average_update_duration /* */ * 99.0 + 1.0 * update_duration /*    */) / 100.0;
      m_statistics.average_cycle_duration   = (m_statistics.average_cycle_duration /*  */ * 99.0 + 1.0 * cycle_duration /*     */) / 100.0;
      utilities::print_ansi_table({
          {"        title", "Boids"},
          {"         tick", m_tick},
          {"update/cycle%", 0100.0 * m_statistics.average_update_duration / m_statistics.average_cycle_duration},
          {"    ms/update", 1000.0 * m_statistics.average_update_duration},
          {"    ms/ cycle", 1000.0 * m_statistics.average_cycle_duration},
          {"    updates/s", 0001.0 / m_statistics.average_update_duration},
          {"     cycles/s", 0001.0 / m_statistics.average_cycle_duration},
          {"ave neighbors", m_statistics.average_neighbors},
          {"max neighbors", m_statistics.max_neighbors},
          {"    subspaces", subspaces.size()},
      });
      m_tick++;
      return static_cast<update_delay>(dt);
    }
    auto on_render() -> void override
    {
      glBindBuffer(GL_ARRAY_BUFFER, m_opengl.vbo);
      if (m_render_tick != m_tick)
      {
        m_render_tick   = m_tick;
        auto const data = std::as_bytes(std::span{m_boids});
        if (m_vbo_bytes_size < data.size() or data.size() * 2zu < m_vbo_bytes_size)
          glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vbo_bytes_size = data.size()), nullptr, GL_DYNAMIC_DRAW), glCheckError();
        glBufferSubData(GL_ARRAY_BUFFER, /* offset */ 0, static_cast<GLsizeiptr>(data.size()), data.data()), glCheckError();
      }

      glUseProgram(m_opengl.pid);
      glUniform1f(m_uniforms.boid_width, m_settings.boid_width);
      glCheckError();

      glBindVertexArray(m_opengl.vao);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, /* vertex index offset */ 0, /* quad vertex count */ 3, static_cast<GLsizei>(m_boids.size()));
      glCheckError();
    }

  private:
    auto inline static constexpr hash_vec = []<glm::length_t L, typename T>(glm::vec<L, T> const p)
    {
      auto res = 0zu;
      for (auto const hash = std::hash<T>{}; auto const i : std::views::iota(glm::length_t{0}, L))
        res = res xor hash(p[i]);
      return res;
    };
    using subspace_id               = glm::i32vec2;
    using boids_grouped_by_subspace = std::unordered_map<subspace_id, std::vector<boid>, decltype(hash_vec)>;
    using distance                  = float;
    using boid_distance_pairs       = std::vector<std::pair<boid, distance>>;

  private:
    simulation_settings       m_settings       = {};
    opengl_handles            m_opengl         = {};
    uniform_locations         m_uniforms       = {};
    statistics                m_statistics     = {};
    size_t                    m_vbo_bytes_size = {}, m_tick = {}, m_render_tick = {};
    std::vector<boid>         m_boids                      = {};
    boids_grouped_by_subspace m_subspaces_allocation_cache = {};
    boid_distance_pairs       m_neighbors_allocation_cache = {};

  private:
    std::string_view m_glsl_version  = {R"glsl(
      #version 300 es
      precision highp float;
      precision highp sampler2DArray;
    )glsl"},
                     m_glsl_vertex   = {R"glsl(
      in      vec2  position;
      in      vec2  velocity;
      in      vec2  acceleration;
      out     vec4  fragment_color;
      uniform float boid_width;
      vec2  quad_positions[3]  = vec2[3](
        vec2(+0.5f, +0.0f),
        vec2(-0.5f, +0.5f),
        vec2(-0.5f, -0.5f)
      );
      vec4  quad_colors[3]     = vec4[3](
        vec4(0.4f, 0.9f, 0.4f, 1.0f),
        vec4(0.9f, 0.3f, 0.2f, 1.0f),
        vec4(0.9f, 0.2f, 0.3f, 1.0f)
      );
      void main()
      {
        vec2  quad_position    = quad_positions[gl_VertexID % 3];
        vec4  quad_color       = quad_colors   [gl_VertexID % 3];
        float angle            = -atan(velocity.y, velocity.x);
        mat2  rotate           = mat2(cos(angle), -sin(angle),
                                      sin(angle),  cos(angle));
        gl_Position            = vec4(position + rotate * quad_position * boid_width, 0.0f, 1.0f);
        fragment_color         = quad_color;
      }
    )glsl"},
                     m_glsl_fragment = {R"glsl(
      in  vec4 fragment_color;
      out vec4 color;
      void main()
      {
        color = fragment_color;
      }
    )glsl"};
};
template <>
auto game::layers::push_layer<game::layers::boids>(bool game_layers, engine::application &app) -> void
{
  if (not game_layers)
  {
    app.schedule_layer_push<boids>();
  }
  else
  {
    app.schedule_layer_push<clear>(-1, glm::vec4{0.15f, 0.05f, 0.5f, 1.0f});
    app.schedule_layer_push<boids>();
  }
}
