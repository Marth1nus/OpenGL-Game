#include <game/game_of_life.hpp>

auto game::layers::game_of_life::simulation_settings::validate() const -> void
{
  auto static constexpr verify_sorted = [](std::string_view const name, auto const &values) static
  { return runtime_assert(std::ranges::is_sorted(values), "invalid {}", name); };
  verify_sorted("width" /*             */, std::array{4zu, width /*             */, 0x04'00zu});
  verify_sorted("height" /*            */, std::array{4zu, height /*            */, 0x04'00zu});
  verify_sorted("tick rate" /*         */, std::array{1zu, tick_rate /*         */, 60zu});
  verify_sorted("init distribution" /* */, std::array{0.0, init_distribution /* */, 100.0});
}
/**/ game::layers::game_of_life::game_of_life(simulation_settings const &settings) : m_settings{(settings.validate(), settings)}
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
/**/ game::layers::game_of_life::~game_of_life()
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
auto game::layers::game_of_life::setup() -> void
{
  glBindVertexArray(m_handles.vao);
  glCheckError();

  auto       cell_rd        = std::random_device{};
  auto       cell_dist      = std::uniform_real_distribution{0.0, 100.0};
  auto const cell_threshold = m_settings.init_distribution;
  for (auto const &[tid, fbo] : {std::pair{m_handles.tid0, m_handles.fbo0},
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
auto game::layers::game_of_life::on_update() -> update_delay
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
  m_tick++;
  return update_delay(1.0) / m_settings.tick_rate;
}
auto game::layers::game_of_life::on_render() -> void
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
auto game::layers::game_of_life::on_imgui() -> void
{
  if (ImGui::BeginTable("stats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
  {
    ImGui::TableSetupColumn("Property");
    ImGui::TableSetupColumn("Value");
    ImGui::TableHeadersRow();
    for (auto const &[title, value_variant] : std::initializer_list<std::tuple<char const *, std::variant<char const *, size_t, double>>>{
             {"        title", "Game Of Life"},
             {"         tick", m_tick},
             {"update/cycle%", 0100.0 * m_statistics.average_update_duration / m_statistics.average_cycle_duration},
             {"    ms/update", 1000.0 * m_statistics.average_update_duration},
             {"    ms/ cycle", 1000.0 * m_statistics.average_cycle_duration},
             {"    updates/s", 0001.0 / m_statistics.average_update_duration},
             {"     cycles/s", 0001.0 / m_statistics.average_cycle_duration},
         })
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::Text("%s", title);
      ImGui::TableSetColumnIndex(1);
      std::visit(
          []<typename T>(T value)
          {
            /**/ if constexpr (std::is_convertible_v<T, char const *>)
              ImGui::Text("%s", static_cast<char const *>(value));
            else if constexpr (std::unsigned_integral<T>)
              ImGui::Text("%zu", static_cast<std::make_unsigned_t<size_t>>(value));
            else if constexpr (std::signed_integral<T>)
              ImGui::Text("%z", static_cast<std::make_signed_t<size_t>>(value));
            else if constexpr (std::floating_point<T>)
              ImGui::Text("%.3f", static_cast<double>(value));
          },
          value_variant);
    }
  }
  ImGui::EndTable();
}
