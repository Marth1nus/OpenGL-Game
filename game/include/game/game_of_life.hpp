#ifndef GAME_GAME_OF_LIFE_HPP
#define GAME_GAME_OF_LIFE_HPP

#include <game/game.hpp>
#include <game/imgui.hpp>

namespace game::layers { struct game_of_life; }
struct game::layers::game_of_life : layer, imgui::client
{
  public:
    struct simulation_settings
    {
        size_t /*    */ width             = 128zu,
                        height            = 128zu,
                        tick_rate         = 30zu;
        double /*    */ init_distribution = 30.0;
        glm::vec4 /* */ color_alive       = {0.0f, 0.8f, 0.6f, 1.0f},
                        color_dead        = {0.0f, 0.0f, 0.0f, 1.0f};

      public:
        auto validate() const -> void;
    };
    struct opengl_handles
    {
        uint32_t vao{}, tid0{}, tid1{}, fbo0{}, fbo1{}, vid{}, fid{}, pid{};
    };
    struct uniform_locations
    {
        int32_t tex = -1, tex_size = -1, print = -1, color_alive = -1, color_dead = -1;
    };
    struct statistics
    {
        std::chrono::steady_clock::time_point
               cycle_start             = std::chrono::steady_clock::now();
        double average_cycle_duration  = 0.0,
               average_update_duration = 0.0;
    };

  public:
    /**/ game_of_life() : game_of_life(simulation_settings{}) {}
    /**/ game_of_life(simulation_settings const &settings);
    /**/ ~game_of_life();

  private:
    auto setup() -> void;

  public:
    auto on_update() -> update_delay override;
    auto on_render() -> void override;
    auto on_imgui() -> void override;

  private:
    simulation_settings m_settings   = {};
    opengl_handles      m_handles    = {};
    uniform_locations   m_uniforms   = {};
    statistics          m_statistics = {};
    size_t              m_tick       = {};

  private:
    std::string_view m_glsl_version  = {R"glsl(
      #version 300 es
      precision highp float;
      precision highp sampler2DArray;
    )glsl"},
                     m_glsl_vertex   = {R"glsl(
      vec2 quad[4] = vec2[4](
        vec2(-1.0f, -1.0f),
        vec2(+1.0f, -1.0f),
        vec2(-1.0f, +1.0f),
        vec2(+1.0f, +1.0f)
      );
      out vec2 uv;
      void main()
      {
        vec2 position = quad[gl_VertexID & 3];
        uv            = position / 2.0f + 0.5f;
        gl_Position   = vec4(position, 0.0f, 1.0f);
      }
    )glsl"},
                     m_glsl_fragment = {R"glsl(
      in      vec2      uv;
      uniform sampler2D tex;
      uniform ivec2     tex_size;
      uniform bool      print;
      uniform vec4      color_alive;
      uniform vec4      color_dead;
      out     vec4      color;
      void main()
      {
        if (print)
        {
          bool cell_alive = texture(tex, uv).r > 0.5f;
          color           = cell_alive ? color_alive : color_dead;
          return;
        }

        ivec2 pos        = ivec2(gl_FragCoord.xy);
        float cell       = texelFetch(tex, pos, 0).r;
        bool  cell_alive = cell > 0.5f;

        int neighbor_count = 0;
        for (int i = -1; i <= 1; i++)
        {
          for (int j = -1; j <= 1; j++)
          {
            if (i == 0 && j == 0) continue;
            ivec2 neighbor_pos = (pos + ivec2(i, j)) % tex_size;
            if (texelFetch(tex, neighbor_pos, 0).r > 0.5f) neighbor_count++;
          }
        }

        cell_alive = cell_alive ? neighbor_count == 2 || neighbor_count == 3
                                : neighbor_count == 3;
        color      = vec4(cell_alive ? 1.0f : 0.0f);
        color.a    = 1.0f;
      }
    )glsl"};
};

#endif // GAME_GAME_OF_LIFE_HPP