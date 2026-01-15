#include <engine/renderer.hpp>

auto engine::renderer::handle_cache::set_capacity(size_t capacity) -> void
{
  if (m_capacity == capacity) return;
  runtime_assert(capacity <= std::numeric_limits<decltype(m_capacity)>::max(), "max possible capacity exceeded");
  runtime_assert(capacity >= m_size, "attempt to delete active handles");
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
  if (m_size >= m_capacity) reserve(m_capacity + s_minimum_batch_reserve_size);
  return m_handles[m_size++];
}
auto engine::renderer::handle_cache::deactivate(uint32_t handle) -> void
{
  auto const handles = all();
  auto const it      = std::ranges::find(handles, handle);
  runtime_assert(it != handles.end(), "handle {} was in not handle cache", handle);
  std::swap(handles[it - handles.begin()], handles[--m_size]);
}

auto engine::renderer::compile_shader(uint32_t shader, std::span<std::string_view const> shader_sources) -> void
{
  runtime_assert(not shader_sources.empty(), "can not compile a shader from no sources");
  auto const version_offset             = shader_sources.front().find("#version");
  auto static constexpr get_string_size = [](std::string_view sv) static
  { return static_cast<GLsizei>(sv.size()); };
  runtime_assert(version_offset != std::string_view::npos, "Shader must start with a #version directive: \n{}", shader_sources.front());
  auto mbr_buf     = std::array<std::byte, (sizeof(char const *) + sizeof(GLsizei)) * 8>{};
  auto mbr         = std::pmr::monotonic_buffer_resource{mbr_buf.data(), mbr_buf.size()};
  auto sources     = std::pmr::vector<char const *>{std::from_range, shader_sources | std::views::transform(&std::string_view::data), &mbr};
  auto lengths     = std::pmr::vector<GLsizei /**/>{std::from_range, shader_sources | std::views::transform(get_string_size /*   */), &mbr};
  sources.front() += static_cast<int64_t>(version_offset);
  lengths.front() -= static_cast<int32_t>(version_offset);
  glShaderSource(shader, static_cast<GLsizei>(shader_sources.size()), sources.data(), lengths.data());
  glCompileShader(shader);
  if (GLint status, length; glGetShaderiv(shader, GL_COMPILE_STATUS, &status), not status)
  {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    auto const log = std::make_unique<char[]>(length);
    glGetShaderInfoLog(shader, length, &length, log.get());
    runtime_assert(false, "{} Error: {}", "Shader Compile", std::string_view{log.get(), static_cast<size_t>(length)});
  }
  glCheckError();
}
auto engine::renderer::link_program(uint32_t program, std::span<uint32_t const> shaders) -> void
{
  for (auto shader : shaders) glAttachShader(program, shader);
  glLinkProgram(program);
  if (GLint status, length; glGetProgramiv(program, GL_LINK_STATUS, &status), not status)
  {
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    auto const log = std::make_unique<char[]>(length);
    glGetProgramInfoLog(program, length, &length, log.get());
    runtime_assert(false, "{} Error: {}", "Program Link", std::string_view{log.get(), static_cast<size_t>(length)});
  }
  glCheckError();
}