#include <game/game.hpp>

struct game::layers::game_of_life : layer
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
        auto validate() const -> void
        {
          auto static constexpr verify_sorted = [](std::string_view const name, auto const &values) static
          { return runtime_assert(std::ranges::is_sorted(values), "invalid {}", name); };
          verify_sorted("width" /*             */, std::array{4zu, width /*             */, 0x04'00zu});
          verify_sorted("height" /*            */, std::array{4zu, height /*            */, 0x04'00zu});
          verify_sorted("tick rate" /*         */, std::array{1zu, tick_rate /*         */, 60zu});
          verify_sorted("init distribution" /* */, std::array{0.0, init_distribution /* */, 100.0});
        }
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
    /**/ game_of_life(simulation_settings const &settings) : m_settings{(settings.validate(), settings)}
    {
      m_handles.vao /*  */ = app().get_renderer().vertexarrays /* */.activate();
      m_handles.tid0 /* */ = app().get_renderer().textures /*     */.activate();
      m_handles.tid1 /* */ = app().get_renderer().textures /*     */.activate();
      m_handles.fbo0 /* */ = app().get_renderer().framebuffers /* */.activate();
      m_handles.fbo1 /* */ = app().get_renderer().framebuffers /* */.activate();
      m_handles.vid /*  */ = glCreateShader(GL_VERTEX_SHADER);
      m_handles.fid /*  */ = glCreateShader(GL_FRAGMENT_SHADER);
      m_handles.pid /*  */ = glCreateProgram();
      setup();
    }
    /**/ ~game_of_life()
    {
      m_handles.vao /*  */ = (app().get_renderer().vertexarrays /* */.deactivate(m_handles.vao /*  */), 0u);
      m_handles.tid0 /* */ = (app().get_renderer().textures /*     */.deactivate(m_handles.tid0 /* */), 0u);
      m_handles.tid1 /* */ = (app().get_renderer().textures /*     */.deactivate(m_handles.tid1 /* */), 0u);
      m_handles.fbo0 /* */ = (app().get_renderer().framebuffers /* */.deactivate(m_handles.fbo0 /* */), 0u);
      m_handles.fbo1 /* */ = (app().get_renderer().framebuffers /* */.deactivate(m_handles.fbo1 /* */), 0u);
      m_handles.vid /*  */ = (glDeleteShader(m_handles.vid), 0u);
      m_handles.fid /*  */ = (glDeleteShader(m_handles.fid), 0u);
      m_handles.pid /*  */ = (glDeleteProgram(m_handles.pid), 0u);
    }

  private:
    auto setup() -> void
    {
      glBindVertexArray(m_handles.vao);
      glCheckError();

      auto       cell_rd        = std::random_device{};
      auto       cell_dist      = std::uniform_real_distribution{0.0, 100.0};
      auto const cell_threshold = m_settings.init_distribution;
      for (auto const [tid, fbo] : {std::pair{m_handles.tid0, m_handles.fbo0},
                                    std::pair{m_handles.tid1, m_handles.fbo1}})
      {
        auto const width  = static_cast<GLsizei>(m_settings.width);
        auto const height = static_cast<GLsizei>(m_settings.height);
        glBindTexture(GL_TEXTURE_2D, tid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glCheckError();

        auto const subpixels = std::views::iota(0, 1 * width * height) |
                               std::views::transform([&]<typename limits = std::numeric_limits<uint8_t>>(size_t)
                                                     { return cell_dist(cell_rd) > cell_threshold ? limits::max() : limits::min(); }) |
                               std::ranges::to<std::vector>();
        glTexImage2D(GL_TEXTURE_2D, /* level */ 0, GL_R8, width, height, /* border */ 0, GL_RED, GL_UNSIGNED_BYTE, subpixels.data());
        glCheckError();

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tid, /* level */ 0);
        runtime_assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete!");
        glCheckError();
      }

      app().get_renderer().compile_shader(m_handles.vid, std::array{m_glsl_version, m_glsl_vertex});
      app().get_renderer().compile_shader(m_handles.fid, std::array{m_glsl_version, m_glsl_fragment});
      app().get_renderer().link_program(m_handles.pid, std::array{m_handles.vid, m_handles.fid});

      m_uniforms = {
          .tex         = glGetUniformLocation(m_handles.pid, "tex" /*         */),
          .tex_size    = glGetUniformLocation(m_handles.pid, "tex_size" /*    */),
          .print       = glGetUniformLocation(m_handles.pid, "print" /*       */),
          .color_alive = glGetUniformLocation(m_handles.pid, "color_alive" /* */),
          .color_dead  = glGetUniformLocation(m_handles.pid, "color_dead" /*  */),
      };

      m_tick = 0zu;
    }

  public:
    auto update() -> update_delay override
    {
      auto const update_start = std::chrono::steady_clock::now();
      auto const even_tick    = m_tick % 2 == 0;
      auto const tid          = even_tick ? m_handles.tid0 : m_handles.tid1;
      auto const fbo          = even_tick ? m_handles.fbo1 : m_handles.fbo0;

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tid);
      glCheckError();

      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glCheckError();

      glUseProgram(m_handles.pid);
      glUniform1i(m_uniforms.tex /*         */, 0);
      glUniform2i(m_uniforms.tex_size /*    */, static_cast<int>(m_settings.width), static_cast<int>(m_settings.height));
      glUniform1i(m_uniforms.print /*       */, false);
      glUniform4f(m_uniforms.color_alive /* */, m_settings.color_alive /* */.r, m_settings.color_alive /* */.g, m_settings.color_alive /* */.b, m_settings.color_alive /* */.a);
      glUniform4f(m_uniforms.color_dead /*  */, m_settings.color_dead /*  */.r, m_settings.color_dead /*  */.g, m_settings.color_dead /*  */.b, m_settings.color_dead /*  */.a);
      glCheckError();

      glBindVertexArray(m_handles.vao);
      glDrawArrays(GL_TRIANGLE_STRIP, /* first */ 0, /* count */ 4);
      glCheckError();

      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glCheckError();

      auto const update_end                = std::chrono::steady_clock::now();
      auto const cycle_end                 = update_end;
      auto const cycle_start               = std::exchange(m_statistics.cycle_start, cycle_end);
      auto const update_duration           = std::chrono::duration_cast<std::chrono::duration<double>>(update_end /* */ - update_start /* */).count();
      auto const cycle_duration            = std::chrono::duration_cast<std::chrono::duration<double>>(cycle_end /*  */ - cycle_start /*  */).count();
      m_statistics.average_update_duration = (m_statistics.average_update_duration /* */ * 99.0 + 1.0 * update_duration /* */) / 100.0;
      m_statistics.average_cycle_duration  = (m_statistics.average_cycle_duration /*  */ * 99.0 + 1.0 * cycle_duration /*  */) / 100.0;
      utilities::print_table({
          {"         tick", static_cast<double>(m_tick)},
          {"     s/update", /* */ m_statistics.average_update_duration},
          {"    updates/s", 1.0 / m_statistics.average_update_duration},
          {"      s/cycle", /* */ m_statistics.average_cycle_duration},
          {"     cycles/s", 1.0 / m_statistics.average_cycle_duration},
          {" update/cycle", /* */ m_statistics.average_update_duration / m_statistics.average_cycle_duration},
      });

      m_tick++;
      return update_delay(1.0) / m_settings.tick_rate;
    }
    auto render() -> void override
    {
      auto const even_tick = m_tick % 2 == 0;
      auto const tid       = even_tick ? m_handles.tid0 : m_handles.tid1;

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, tid);
      glCheckError();

      glUseProgram(m_handles.pid);
      glUniform1i(m_uniforms.tex /*         */, 0);
      glUniform2i(m_uniforms.tex_size /*    */, static_cast<int>(m_settings.width), static_cast<int>(m_settings.height));
      glUniform1i(m_uniforms.print /*       */, true);
      glUniform4f(m_uniforms.color_alive /* */, m_settings.color_alive /* */.r, m_settings.color_alive /* */.g, m_settings.color_alive /* */.b, m_settings.color_alive /* */.a);
      glUniform4f(m_uniforms.color_dead /*  */, m_settings.color_dead /*  */.r, m_settings.color_dead /*  */.g, m_settings.color_dead /*  */.b, m_settings.color_dead /*  */.a);
      glCheckError();

      glBindVertexArray(m_handles.vao);
      glDrawArrays(GL_TRIANGLE_STRIP, /* first */ 0, /* count */ 4);
      glCheckError();
    }

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
namespace game // make_layer<game_of_life>, push_game<game_of_life>
{
  template <>
  auto layers::make_layer<layers::game_of_life>() -> std::shared_ptr<layer>
  {
    return std::make_shared<game_of_life>();
  }
  template <>
  auto layers::push_game<layers::game_of_life>(engine::application &app) -> void
  {
    app.schedule_layer_push<clear>(-1, glm::vec4{0.0f, 0.5f, 0.0f, 1.0f});
    app.schedule_layer_push<game_of_life>();
  }
} // namespace game
