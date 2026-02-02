#include <engine/renderer.hpp>

auto engine::renderer::handle_cache::set_capacity(size_t capacity) -> void
{
  if (m_capacity == capacity) return;
  runtime_assert(capacity <= std::numeric_limits<decltype(m_capacity)>::max(), "max possible capacity exceeded");
  runtime_assert(capacity >= m_size, "set_capacity({}) would delete {} active handles", capacity, static_cast<decltype(capacity)>(m_size) - capacity);
  runtime_assert(m_allocator, "null {} access", "allocator");
  auto const old         = all();
  auto const old_handles = std::move(m_handles);
  m_handles              = capacity ? std::make_unique<uint32_t[]>(capacity) : nullptr;
  m_capacity             = static_cast<decltype(m_capacity)>(capacity);
  std::ranges::copy(old.subspan(0, std::min(capacity, old.size())), m_handles.get());
  /**/ if (capacity < old.size())
    m_allocator->delete_handles(old.subspan(capacity));
  else if (capacity > old.size())
    m_allocator->create_handles(all().subspan(old.size()));
}
auto engine::renderer::handle_cache::reserve(size_t capacity) -> void
{
  if (m_capacity >= capacity) return;
  set_capacity(std::bit_ceil(capacity));
}

auto engine::renderer::handle_cache::activate() -> uint32_t
{
  if (m_size >= m_capacity) reserve(1zu + m_size + s_minimum_batch_reserve_size);
  return m_handles[m_size++];
}
auto engine::renderer::handle_cache::deactivate(uint32_t handle) -> void
{
  auto const active_reverse = std::views::reverse(active());
  auto const it             = std::ranges::find(active_reverse, handle);
  runtime_assert(it != active_reverse.end(), "handle {} not active", handle);
  std::swap(*it, all()[--m_size]);
}

auto engine::renderer::compile_shader(uint32_t shader, std::span<std::string_view const> shader_sources) -> void
{
  runtime_assert(not shader_sources.empty(), "can not compile a shader from no sources");
  auto const version_offset = shader_sources.front().find("#version");
  runtime_assert(version_offset != std::string_view::npos, "Shader must start with a #version directive: \n{}", shader_sources.front());
  auto static constexpr get_string_GLsizei = [](std::string_view sv) static
  { return static_cast<GLsizei>(sv.size()); };
  auto static constexpr inline_reserved   = 16zu;
  alignas(char const *) auto sources_buf  = std::array<std::byte, sizeof(char const *) * inline_reserved>{};
  alignas(GLsizei /**/) auto lengths_buf  = std::array<std::byte, sizeof(GLsizei /**/) * inline_reserved>{};
  auto                       sources_mbr  = std::pmr::monotonic_buffer_resource{sources_buf.data(), std::span{sources_buf}.size_bytes()};
  auto                       lengths_mbr  = std::pmr::monotonic_buffer_resource{lengths_buf.data(), std::span{lengths_buf}.size_bytes()};
  auto                       sources      = std::pmr::vector<char const *>{std::from_range, shader_sources | std::views::transform(&std::string_view::data), &sources_mbr};
  auto                       lengths      = std::pmr::vector<GLsizei /**/>{std::from_range, shader_sources | std::views::transform(get_string_GLsizei /**/), &lengths_mbr};
  sources.front()                        += static_cast<GLsizei>(version_offset);
  lengths.front()                        -= static_cast<GLsizei>(version_offset);
  glShaderSource(shader, static_cast<GLsizei>(shader_sources.size()), sources.data(), lengths.data());
  glCompileShader(shader);
  if (GLint status; glGetShaderiv(shader, GL_COMPILE_STATUS, &status), not status)
  {
    auto [log, length] = std::tuple{std::string{}, GLsizei{}};
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    log.resize_and_overwrite(length, [&](char *buf, size_t) -> size_t
                             { return glGetShaderInfoLog(shader, length, &length, buf), length; });
    runtime_assert(false, "{} Error: {}", "Shader Compile", log);
  }
  glCheckError();
}
auto engine::renderer::link_program(uint32_t program, std::span<uint32_t const> shaders) -> void
{
  for (auto shader : shaders) glAttachShader(program, shader);
  glLinkProgram(program);
  if (GLint status; glGetProgramiv(program, GL_LINK_STATUS, &status), not status)
  {
    auto [log, length] = std::tuple{std::string{}, GLsizei{}};
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    log.resize_and_overwrite(length, [&](char *buf, size_t) -> size_t
                             { return glGetProgramInfoLog(program, length, &length, buf), length; });
    runtime_assert(false, "{} Error: {}", "Program Link", log);
  }
  glCheckError();
}