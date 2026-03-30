#ifndef GAME_BOIDS_HPP
#define GAME_BOIDS_HPP

#include <game/game.hpp>
#include <game/imgui.hpp>

namespace game::layers { struct boids; }
struct game::layers::boids : layer, imgui::client
{
  public:
    struct simulation_settings
    {
        float  min_position      = -1.0f,
               max_position      = +1.0f,
               min_velocity      = +0.1f,
               max_velocity      = +0.5f,
               min_acceleration  = +0.1f,
               max_acceleration  = +2.0f;
        float  view_distance     = +0.1f,
               boid_width        = view_distance / 4.0f;
        float  weight_separation = +0.02f,
               weight_alignment  = +1.2f,
               weight_cohesion   = +1.3f,
               weight_mouse_flee = +100.0f;
        size_t tick_rate         = 60zu,
               boid_count        = 800zu,
               max_neighbors     = 16zu;
        auto inline subspace_width() const noexcept { return view_distance; }
        auto inline subspace_count() const noexcept { return static_cast<glm::i32>(glm::ceil((max_position - min_position) / subspace_width())); }

      public:
        auto validate() const -> void;
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
    /**/ boids(simulation_settings const &settings);
    /**/ ~boids();

  private:
    auto inline static thread_local random = [rd = std::random_device{}]<std::floating_point T = float>(T min = -1.0f, T max = 1.0f) mutable
    { return std::uniform_real_distribution{min, max}(rd); };
    auto setup() -> void;

  public:
    auto on_update() -> update_delay override;
    auto on_render() -> void override;
    auto on_imgui() -> void override;

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
    simulation_settings       m_settings                   = {};
    opengl_handles            m_opengl                     = {};
    uniform_locations         m_uniforms                   = {};
    statistics                m_statistics                 = {};
    size_t                    m_vbo_bytes_size             = {},
                              m_tick                       = {},
                              m_render_tick                = {};
    std::vector<boid>         m_boids                      = {};
    boids_grouped_by_subspace m_subspaces                  = {};
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

#endif // GAME_BOIDS_HPP