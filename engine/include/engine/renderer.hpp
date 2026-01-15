#ifndef ENGINE_RENDERER_HPP
#define ENGINE_RENDERER_HPP

#include <engine/core.hpp>
#include <engine/utilities.hpp>

#if /* */ defined(glCheckError) or defined(LAMBDA)
#error "Macro name collision"
#endif // defined(glCheckError) or defined(LAMBDA)

#if /* */ defined(_WIN32) or defined(_WIN64)
#include <glad/glad.h>
#else  // defined(_WIN32) or defined(_WIN64)
#include <GLES3/gl3.h>
#endif // defined(_WIN32) or defined(_WIN64)

#define glCheckError ::engine::utilities::gl::check_error

namespace engine::utilities::gl
{
  struct error : std::runtime_error
  {
      using std::runtime_error::runtime_error;
  };
  auto /*  */ constexpr error_to_string(GLenum error) noexcept -> std::string_view
  {
    switch (error)
    {
      case GL_NO_ERROR /*                      */: return "NO_ERROR" /*                      */;
      case GL_INVALID_ENUM /*                  */: return "INVALID_ENUM" /*                  */;
      case GL_INVALID_VALUE /*                 */: return "INVALID_VALUE" /*                 */;
      case GL_INVALID_OPERATION /*             */: return "INVALID_OPERATION" /*             */;
      case GL_INVALID_FRAMEBUFFER_OPERATION /* */: return "INVALID_FRAMEBUFFER_OPERATION" /* */;
      case GL_OUT_OF_MEMORY /*                 */: return "OUT_OF_MEMORY" /*                 */;
      default /*                               */: return "UNKNOWN" /*                       */;
    }
  }
  auto inline /*     */ check_error(std::source_location source_location = std::source_location::current()) -> void
  {
    while (auto const e = glGetError() - GL_NO_ERROR)
      runtime_assert<error>(false, "OpenGL Error: \"{}\" from {}:{}:{} in function {}", error_to_string(e + GL_NO_ERROR).data(), source_location.file_name(), source_location.line(), source_location.column(), source_location.function_name());
  }
} // namespace engine::utilities::gl
namespace engine
{
  struct renderer
  {
    public:
      struct handle_cache
      {
        public:
          auto inline static constexpr s_minimum_batch_reserve_size = 4zu;
          struct allocator
          {
              auto (*create_handles)(std::span<uint32_t> target) noexcept -> void = 0;
              auto (*delete_handles)(std::span<uint32_t> target) noexcept -> void = 0;
          };
          struct allocators
          {
#define LAMBDA(fn) +[](std::span<uint32_t> target) static noexcept -> void { (fn)(static_cast<GLsizei>(target.size()), target.data()); }
              auto inline static constexpr buffers /*       */ = allocator{.create_handles = LAMBDA(glGenBuffers /*       */), .delete_handles = LAMBDA(glDeleteBuffers /*       */)};
              auto inline static constexpr framebuffers /*  */ = allocator{.create_handles = LAMBDA(glGenFramebuffers /*  */), .delete_handles = LAMBDA(glDeleteFramebuffers /*  */)};
              auto inline static constexpr renderbuffers /* */ = allocator{.create_handles = LAMBDA(glGenRenderbuffers /* */), .delete_handles = LAMBDA(glDeleteRenderbuffers /* */)};
              auto inline static constexpr textures /*      */ = allocator{.create_handles = LAMBDA(glGenTextures /*      */), .delete_handles = LAMBDA(glDeleteTextures /*      */)};
              auto inline static constexpr vertexarrays /*  */ = allocator{.create_handles = LAMBDA(glGenVertexArrays /*  */), .delete_handles = LAMBDA(glDeleteVertexArrays /*  */)};
#undef LAMBDA
          };

          /**/ inline handle_cache() noexcept = default;
          /**/ inline handle_cache(allocator const *allocator) noexcept : m_allocator{allocator} {}
          /**/ inline handle_cache(handle_cache /* */ &&o) noexcept
          {
            m_allocator = std::exchange(o.m_allocator /* */, {});
            m_handles   = std::exchange(o.m_handles /*   */, {});
            m_size      = std::exchange(o.m_size /*      */, {});
            m_capacity  = std::exchange(o.m_capacity /*  */, {});
          }
          /**/ inline handle_cache(handle_cache const &&o) noexcept = delete;
          auto inline operator=(handle_cache /* */ &&o) -> handle_cache & { return this->~handle_cache(), *new (this) handle_cache{std::move(o)}; }
          auto inline operator=(handle_cache const &o) -> handle_cache & = delete;
          /**/ inline ~handle_cache() { m_size = 0, set_capacity(0); }

          auto /*  */ set_capacity(size_t capacity) -> void;
          auto /*  */ reserve(size_t capacity) -> void;

          auto /*  */ activate() -> uint32_t;
          auto /*  */ deactivate(uint32_t handle) -> void;

          auto inline get_allocator() const noexcept { return m_allocator; }

          auto inline all /*      */ () const noexcept -> std::span<uint32_t const> { return std::span{m_handles.get(), m_capacity}; }
          auto inline active /*   */ () const noexcept -> std::span<uint32_t const> { return all().subspan(0, m_size); }
          auto inline inactive /* */ () const noexcept -> std::span<uint32_t const> { return all().subspan(m_size); }

        private:
          auto inline all /*      */ () /* */ noexcept -> std::span<uint32_t /* */> { return std::span{m_handles.get(), m_capacity}; }
          auto inline active /*   */ () /* */ noexcept -> std::span<uint32_t /* */> { return all().subspan(0, m_size); }
          auto inline inactive /* */ () /* */ noexcept -> std::span<uint32_t /* */> { return all().subspan(m_size); }
          allocator const            *m_allocator = 0;
          std::unique_ptr<uint32_t[]> m_handles   = 0;
          uint32_t /*              */ m_size      = 0,
                                      m_capacity  = 0;
      };
      handle_cache
          buffers /*       */ {&handle_cache::allocators::buffers /*       */},
          framebuffers /*  */ {&handle_cache::allocators::framebuffers /*  */},
          renderbuffers /* */ {&handle_cache::allocators::renderbuffers /* */},
          textures /*      */ {&handle_cache::allocators::textures /*      */},
          vertexarrays /*  */ {&handle_cache::allocators::vertexarrays /*  */};

    public:
      /**/ inline renderer() noexcept                                = default;
      /**/ inline ~renderer() noexcept                               = default;
      /**/ inline renderer(renderer /**/ &&) noexcept                = default;
      /**/ inline renderer(renderer const &) noexcept                = delete;
      auto inline operator=(renderer /**/ &&) noexcept -> renderer & = default;
      auto inline operator=(renderer const &) noexcept -> renderer & = delete;

    public:
      auto static compile_shader(uint32_t shader, std::span<std::string_view const> shader_sources) -> void;
      auto inline compile_shader(uint32_t shader, std::string_view shader_source) { return compile_shader(shader, std::span{&shader_source, 1}); }
      auto static link_program(uint32_t program, std::span<uint32_t const> shaders) -> void;
  };
} // namespace engine

#endif // ENGINE_RENDERER_HPP
