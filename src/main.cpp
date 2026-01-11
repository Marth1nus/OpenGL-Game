#include <algorithm>
#include <array>
#include <concepts>
#include <expected>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <memory_resource>
#include <optional>
#include <print>
#include <random>
#include <ranges>
#include <set>
#include <source_location>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

/*================================*\
|*================================*|
\*================================*/

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/*================================*\
|*================================*|
\*================================*/

#undef USE_GLAD
#define USE_GLAD

#if defined(USE_GLAD)
#include <glad/glad.h>
auto constexpr g_using_glad = true;
#else  // defined(USE_GLAD)
#include <GLES3/gl3.h>
auto constexpr g_using_glad = false;
#endif // defined(USE_GLAD)
#include <glfw/glfw3.h>

#if defined(__EMSCRIPTEN__)
auto constexpr g_using_emscripten = true;
#else  // defined(__EMSCRIPTEN__)
auto constexpr g_using_emscripten = false;
#endif // defined(__EMSCRIPTEN__)

/*================================*\
|*================================*|
\*================================*/

#if defined(LAMBDA) or defined(glCheckError)
#error "Macro name collision"
#endif // defined(LAMBDA) or defined(glCheckError)

/*================================*\
|*================================*|
\*================================*/

namespace utilities
{
  struct runtime_assert_failure : std::runtime_error
  {
      using std::runtime_error::runtime_error;
  };
  template <std::constructible_from<std::string> exception = runtime_assert_failure, std::convertible_to<bool> T, typename... Args>
  auto inline runtime_assert(T &&value, std::format_string<Args...> message_format = "failed runtime assert", Args &&...message_args) -> decltype(value)
  {
    if (value) return std::forward<decltype(value)>(value);
    auto message = std::format(message_format, std::forward<decltype(message_args)>(message_args)...);
    throw exception{std::move(message)};
  }
  auto /*  */ read_all(char const *file_path) noexcept -> std::expected<std::string, std::error_code>
  {
    auto const file     = std::fopen(file_path, "rb");
    auto const file_ptr = std::unique_ptr<std::FILE, decltype(&std::fclose)>{file, &std::fclose};
    auto       len      = decltype(std::ftell(file)){};
    auto       res      = std::string{};
    if (not file_ptr or
        std::fseek(file, 0, SEEK_END) != 0 or
        (len = std::ftell(file), len < 0) or
        std::fseek(file, 0, SEEK_SET) != 0 or
        (res.resize_and_overwrite(len, [file](char *buf, size_t len)
                                  { return std::fread(buf, sizeof(char), len, file); }),
         len != res.size()) or
        std::ferror(file))
      return std::unexpected{std::error_code{errno, std::generic_category()}};
    return {std::move(res)};
  }
} // namespace utilities
using utilities::runtime_assert;

/*================================*\
|*================================*|
\*================================*/

namespace utilities::gl
{
  struct error : std::runtime_error
  {
      using std::runtime_error::runtime_error;
  };
  auto error_to_string(GLenum error) noexcept -> std::string_view
  {
    switch (error)
    {
      case GL_NO_ERROR /*                      */: return "NO_ERROR" /*                      */;
      case GL_INVALID_ENUM /*                  */: return "INVALID_ENUM" /*                  */;
      case GL_INVALID_VALUE /*                 */: return "INVALID_VALUE" /*                 */;
      case GL_INVALID_OPERATION /*             */: return "INVALID_OPERATION" /*             */;
      case GL_INVALID_FRAMEBUFFER_OPERATION /* */: return "INVALID_FRAMEBUFFER_OPERATION" /* */;
      case GL_OUT_OF_MEMORY /*                 */: return "OUT_OF_MEMORY" /*                 */;
      default /*                               */: return "UNKNOWN";
    }
  }
  auto check_error(std::source_location source_location = std::source_location::current()) -> void
  {
    while (auto const e = glGetError() - GL_NO_ERROR)
      runtime_assert<error>(false, "OpenGL Error: {:?} from {}:{}:{} in function {}", error_to_string(e + GL_NO_ERROR), source_location.file_name(), source_location.line(), source_location.column(), source_location.function_name());
  }
} // namespace utilities::gl
#define glCheckError ::utilities::gl::check_error

/*================================*\
|*================================*|
\*================================*/

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
#define LAMBDA(fn) +[](std::span<uint32_t> target) noexcept -> void { (fn)(static_cast<GLsizei>(target.size()), target.data()); }
            auto inline static constexpr buffers /*       */ = allocator{.create_handles = LAMBDA(glGenBuffers /*       */), .delete_handles = LAMBDA(glDeleteBuffers /*       */)};
            auto inline static constexpr framebuffers /*  */ = allocator{.create_handles = LAMBDA(glGenFramebuffers /*  */), .delete_handles = LAMBDA(glDeleteFramebuffers /*  */)};
            auto inline static constexpr renderbuffers /* */ = allocator{.create_handles = LAMBDA(glGenRenderbuffers /* */), .delete_handles = LAMBDA(glDeleteRenderbuffers /* */)};
            auto inline static constexpr textures /*      */ = allocator{.create_handles = LAMBDA(glGenTextures /*      */), .delete_handles = LAMBDA(glDeleteTextures /*      */)};
            auto inline static constexpr vertexarrays /*  */ = allocator{.create_handles = LAMBDA(glGenVertexArrays /*  */), .delete_handles = LAMBDA(glDeleteVertexArrays /*  */)};
#undef LAMBDA
        };

        /**/ handle_cache() noexcept = default;
        /**/ handle_cache(allocator const *allocator) noexcept : m_allocator{allocator} {}
        /**/ handle_cache(handle_cache /* */ &&o) noexcept
        {
          m_allocator = std::exchange(o.m_allocator /* */, {});
          m_handles   = std::exchange(o.m_handles /*   */, {});
          m_size      = std::exchange(o.m_size /*      */, {});
          m_capacity  = std::exchange(o.m_capacity /*  */, {});
        }
        /**/ handle_cache(handle_cache const &&o) noexcept = delete;
        auto operator=(handle_cache /* */ &&o) -> handle_cache & { return this->~handle_cache(), *new (this) handle_cache{std::move(o)}; }
        auto operator=(handle_cache const &o) -> handle_cache & = delete;
        /**/ ~handle_cache() { m_size = 0, set_capacity(0); }

        auto /*  */ set_capacity(size_t capacity) -> void
        {
          if (m_capacity == capacity) return;
          runtime_assert(capacity <= std::numeric_limits<decltype(m_capacity)>::max(), "max possible capacity exceeded");
          runtime_assert(capacity >= m_size, "attempt to delete active handles");
          runtime_assert(m_allocator, "null {} access", "allocator");
          auto const old = all();
          m_handles      = capacity ? new uint32_t[capacity] : nullptr;
          m_capacity     = static_cast<decltype(m_capacity)>(capacity);
          std::ranges::copy(old.subspan(0, std::min(capacity, old.size())), m_handles);
          /**/ if (capacity < old.size())
            m_allocator->delete_handles(old.subspan(capacity));
          else if (capacity > old.size())
            m_allocator->create_handles(all().subspan(old.size()));
          if (not old.empty()) delete[] old.data();
        }
        auto /*  */ reserve(size_t capacity) -> void
        {
          if (m_capacity >= capacity) return;
          set_capacity(std::bit_ceil(capacity));
        }

        auto /*  */ activate() -> uint32_t
        {
          if (m_size >= m_capacity) reserve(m_capacity + s_minimum_batch_reserve_size);
          return m_handles[m_size++];
        }
        auto /*  */ deactivate(uint32_t handle) -> void
        {
          auto const handles = all();
          auto const it      = std::ranges::find(handles, handle);
          runtime_assert(it != handles.end(), "handle {} was in not handle cache", handle);
          std::swap(handles[it - handles.begin()], handles[--m_size]);
        }

        auto inline get_allocator() const noexcept { return m_allocator; }

        auto inline all /*      */ () const noexcept -> std::span<uint32_t const> { return std::span{m_handles, m_capacity}; }
        auto inline active /*   */ () const noexcept -> std::span<uint32_t const> { return all().subspan(0, m_size); }
        auto inline inactive /* */ () const noexcept -> std::span<uint32_t const> { return all().subspan(m_size); }

      private:
        auto inline all /*      */ () /* */ noexcept -> std::span<uint32_t /* */> { return std::span{m_handles, m_capacity}; }
        auto inline active /*   */ () /* */ noexcept -> std::span<uint32_t /* */> { return all().subspan(0, m_size); }
        auto inline inactive /* */ () /* */ noexcept -> std::span<uint32_t /* */> { return all().subspan(m_size); }
        allocator const *m_allocator = 0;
        uint32_t        *m_handles   = 0,
                 m_size              = 0,
                 m_capacity          = 0;
    };
    handle_cache
        buffers /*       */ {&handle_cache::allocators::buffers /*       */},
        framebuffers /*  */ {&handle_cache::allocators::framebuffers /*  */},
        renderbuffers /* */ {&handle_cache::allocators::renderbuffers /* */},
        textures /*      */ {&handle_cache::allocators::textures /*      */},
        vertexarrays /*  */ {&handle_cache::allocators::vertexarrays /*  */};

  public:
    /**/ renderer() {}
    /**/ ~renderer() {}
    /**/ renderer(renderer /**/ &&) noexcept                = default;
    /**/ renderer(renderer const &) noexcept                = delete;
    auto operator=(renderer /**/ &&) noexcept -> renderer & = default;
    auto operator=(renderer const &) noexcept -> renderer & = delete;

  public:
    auto static compile_shader(uint32_t shader, std::span<std::string_view const> shader_sources)
    {
      runtime_assert(not shader_sources.empty(), "can not compile a shader from no sources");
      auto const version_offset             = shader_sources.front().find("#version");
      auto static constexpr get_string_size = [](std::string_view sv)
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
    auto inline compile_shader(uint32_t shader, std::string_view shader_source) { return compile_shader(shader, std::span{&shader_source, 1}); }
    auto static link_program(uint32_t program, std::span<uint32_t const> shaders)
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
};

/*================================*\
|*================================*|
\*================================*/

struct application
{
  public:
    struct layer
    {
        /**/ virtual ~layer() noexcept = 0 {}
        auto virtual update() -> void  = 0;
        auto virtual render() -> void  = 0;
    };
    using layer_t       = layer;
    using layers_t      = std::vector<std::shared_ptr<layer_t>>;
    using layers_task_t = std::function<void(layers_t &layers)>;

  public:
    auto inline static get() noexcept -> auto & { return *runtime_assert(s_instance, "null {} access", "application"); }

    /**/ application()
    {
      runtime_assert(not s_instance, "application singleton violation");
      s_instance = this;
      runtime_assert(glfwInit(), "{} init fail", "glfw");
      m_window = glfwCreateWindow(720, 720, "game", nullptr, nullptr);
      runtime_assert(m_window, "{} init fail", "window");
      glfwMakeContextCurrent(m_window);
      if constexpr (g_using_glad) runtime_assert(
          gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)), "{} init fail", "glad");
    }
    /**/ ~application()
    {
      m_layers_tasks = {};
      m_layers       = {};
      m_renderer     = {};
      m_window       = (glfwDestroyWindow(m_window), nullptr);
      glfwTerminate();
      runtime_assert(s_instance == this, "application singleton violation");
      s_instance = nullptr;
    }

    template <std::convertible_to<layers_task_t> T>
    auto inline schedule_layer_manipulation(T &&task) -> void { m_layers_tasks.emplace_back(std::forward<T>(task)); }
    template <std::invocable T>
      requires std::convertible_to<std::invoke_result_t<T>, std::shared_ptr<layer_t>>
    auto inline schedule_layer_push(T &&make_layer, std::ptrdiff_t index = -1) -> void
    {
      schedule_layer_manipulation(
          [make_layer = std::forward<T>(make_layer), index](layers_t &layers) mutable
          {
            auto layer = static_cast<std::shared_ptr<layer_t>>(std::invoke(make_layer));
            runtime_assert(std::ranges::find(layers, layer) == layers.end(), "layer {} {} in layer stack", static_cast<void const *>(layer.get()), "already");
            auto const i = index < 0 ? index + layers.size() + 1 : index;
            runtime_assert(0 <= i and i <= layers.size(), "out of range");
            layers.emplace(layers.begin() + i, std::move(layer));
          });
    }
    template <std::derived_from<layer_t> T, typename... Args>
      requires std::is_constructible_v<T, std::remove_cvref_t<Args> &&...>
    auto inline schedule_layer_push(std::ptrdiff_t index = -1, Args &&...args) -> void
    {
      return schedule_layer_push([args_tuple = std::tuple{std::forward<Args>(args)...}]() mutable
                                 { return std::apply([](auto &...args)
                                                     { return std::make_shared<T>(std::move(args)...); },
                                                     args_tuple); },
                                 index);
    }
    auto /*  */ schedule_layer_pop(std::shared_ptr<layer_t const> layer) -> void
    {
      schedule_layer_manipulation(
          [layer = std::move(layer)](layers_t &layers) mutable
          {
            auto const it = std::ranges::find(layers, layer);
            runtime_assert(it != layers.end(), "layer {} {} in layer stack", static_cast<void const *>(layer.get()), "not");
            layers.erase(it);
            layer = {}; // layer destruct here
          });
    }

    auto inline get_window() const -> auto & { return *runtime_assert(m_window, "null {} access", "main window"); }
    auto inline get_renderer() const noexcept -> auto & { return m_renderer; }
    auto inline get_renderer() /* */ noexcept -> auto & { return m_renderer; }
    auto inline get_layers() const noexcept { return std::span{m_layers}; }

    auto /*  */ run() -> int
    {
      while (not glfwWindowShouldClose(m_window))
      {
        { // layer tasks
          for (auto &task : std::exchange(m_layers_tasks, {})) try
            {
              task(m_layers);
              runtime_assert(m_layers_tasks.empty(), "{0:?} can not be scheduled from a {0:?} task", "between frame layer manipulation");
            }
            catch (std::exception const &e)
            {
              std::println(stderr, "Error in {:?}: {}", "between frame layer manipulation", e.what());
              m_layers.clear(), m_layers_tasks.clear();
            };
        }
        glfwPollEvents();
        for (auto const &layer : get_layers()) layer->update();
        for (auto const &layer : get_layers()) layer->render();
        glfwSwapBuffers(m_window);
      }
      return EXIT_SUCCESS;
    }

  private:
    auto inline static s_instance = (application *)nullptr;
    GLFWwindow                *m_window{};
    renderer                   m_renderer{};
    layers_t                   m_layers{};
    std::vector<layers_task_t> m_layers_tasks{};
};

/*================================*\
|*================================*|
\*================================*/

namespace layers
{
  using layer = application::layer;
  struct clear;
  struct boids;
} // namespace layers

/*================================*\
|*================================*|
\*================================*/

struct layers::clear : layer
{
    glm::vec4 color{0.1f, 0.1f, 0.1f, 1.0f};
    /**/ clear(glm::vec4 color) noexcept : color{color} {}
    /**/ clear() noexcept                             = default;
    /**/ clear(clear const &) noexcept                = default;
    auto operator=(clear const &) noexcept -> clear & = default;
    /**/ ~clear() noexcept override {}
    auto update() -> void override {}
    auto render() -> void override
    {
      glClearColor(color.r, color.g, color.b, color.a);
      glClear(GL_COLOR_BUFFER_BIT);
    }
};

/*================================*\
|*================================*|
\*================================*/

struct layers::boids : layer
{
  public:
    struct boid
    {
        uint32_t              id{}, padding{};
        glm::vec2             position{}, velocity{}, acceleration{};
        auto inline constexpr operator<=>(boid const &) const noexcept -> auto = default;
    };

  public:
    /**/ boids()
    {
      m_pid = glCreateProgram();
      m_vid = glCreateShader(GL_VERTEX_SHADER);
      m_fid = glCreateShader(GL_FRAGMENT_SHADER);
      m_vbo = application::get().get_renderer().buffers.activate();
      m_vao = application::get().get_renderer().vertexarrays.activate();
      verify_values();
      setup();
    }
    /**/ ~boids() override
    {
      m_pid = (glDeleteProgram(m_pid), 0);
      m_vid = (glDeleteShader(m_vid), 0);
      m_fid = (glDeleteShader(m_fid), 0);
      m_vbo = (application::get().get_renderer().buffers.deactivate(m_vbo), 0);
      m_vao = (application::get().get_renderer().vertexarrays.deactivate(m_vao), 0);
    }

  private:
    auto verify_values() -> void
    {
      runtime_assert(/*                                */ m_min_position /*     */ <= m_max_position /*     */, "{} limits invalid", "position" /*     */);
      runtime_assert(0.0f <= m_min_velocity /*     */ and m_min_velocity /*     */ <= m_max_velocity /*     */, "{} limits invalid", "velocity" /*     */);
      runtime_assert(0.0f <= m_min_acceleration /* */ and m_min_acceleration /* */ <= m_max_acceleration /* */, "{} limits invalid", "acceleration" /* */);
    }
    auto setup() -> void
    {
      glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
      glBufferData(GL_ARRAY_BUFFER, /* size */ 1, nullptr, GL_STATIC_DRAW); // temp buffer data

      glBindVertexArray(m_vao);
      for (auto attrib_index = 0; auto const member : {&boid::position, &boid::velocity, &boid::acceleration})
      {
        auto const offset = &(((boid const *)0)->*member);
        glVertexAttribPointer(attrib_index, (GLint)offset->length(), GL_FLOAT, GL_FALSE, (GLsizei)sizeof(boid), offset);
        glVertexAttribDivisor(attrib_index, 1);
        glEnableVertexAttribArray(attrib_index++);
        glCheckError();
      };

      application::get().get_renderer().compile_shader /* */ (m_vid, std::array{m_glsl_version, m_glsl_vertex});
      application::get().get_renderer().compile_shader /* */ (m_fid, std::array{m_glsl_version, m_glsl_fragment});
      application::get().get_renderer().link_program /*   */ (m_pid, std::array{m_vid, m_fid});

      glUseProgram(m_pid);
      glUniform1f(m_uniforms.boid_width = glGetUniformLocation(m_pid, "boid_width"), m_boid_width);
      glCheckError();

      m_boids.clear();
      m_time        = static_cast<float>(glfwGetTime());
      m_vbo_size    = 0;
      m_tick        = 0;
      m_render_tick = 0;

      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    auto static random() -> float
    {
      auto thread_local rd = std::random_device{};
      return std::uniform_real_distribution<float>{-1.0f, 1.0f}(rd);
    }

  public:
    auto update() -> void override
    {
      auto const [time, dt, skip] = [&]
      {
        auto const dt        = 1.0f / static_cast<float>(m_tick_rate);
        auto const time      = static_cast<float>(glfwGetTime());
        auto const elapsed   = time - m_time;
        auto const tick_time = 1.0f / m_tick_rate;
        auto const behind    = elapsed / tick_time;
        if (behind < 01.0f) return std::tuple{time, dt, true};
        if (behind > 10.0f) m_time = time - tick_time;
        m_time += tick_time;
        m_tick += 1;
        return std::tuple{time, dt, false};
      }();
      if (skip) return;

      auto const [mouse_pos] = [&] { // and viewport update
        int window_width, window_height;
        glfwGetWindowSize(&application::get().get_window(), &window_width, &window_height);
        auto const vmax     = std::max(window_width, window_height);
        auto const window_x = (window_width - vmax) / 2;
        auto const window_y = (window_height - vmax) / 2;
        glViewport(window_x, window_y, vmax, vmax);

        double mouse_x, mouse_y;
        glfwGetCursorPos(&application::get().get_window(), &mouse_x, &mouse_y);
        auto const mouse_pos = glm::vec2{
            /* */ ((mouse_x - window_x) / vmax) * 2.0 - 1.0,
            1.0 - ((mouse_y - window_y) / vmax) * 2.0 /* */,
        };
        return std::tuple{mouse_pos};
      }();

      if (m_boids.size() != m_boid_count)
      {
        auto const old_boid_count = m_boids.size();
        m_boids.resize(m_boid_count);
        for (auto &boid : m_boids | std::views::drop(old_boid_count))
        {
          boid.id           = static_cast<decltype(boid.id)>(&boid - m_boids.data());
          boid.position     = glm::vec2{random(), random()} * m_max_position;
          boid.velocity     = glm::vec2{random(), random()} * m_min_velocity;
          boid.acceleration = glm::vec2{random(), random()} * m_min_acceleration;
        }
      }

      for (auto &[id, boids] : m_boid_subspaces) boids.clear();
      for (auto &boid : m_boids) m_boid_subspaces[glm::floor(boid.position * static_cast<float>(m_subspace_count))].push_back(boid);

      for (auto &boid : m_boids)
      {
        auto const [n_boid_neighbors] = [&]
        {
          m_boid_neighbors.clear();
          auto const subspace_id             = glm::floor(boid.position * static_cast<float>(m_subspace_count));
          auto const subspace_offsets_radius = static_cast<int32_t>(std::ceil(m_view_distance / m_subspace_width));
          auto const subspace_offsets        = std::views::iota(-subspace_offsets_radius, subspace_offsets_radius + 1);
          for (auto const y : subspace_offsets)
            for (auto const x : subspace_offsets)
              for (auto const skip_self_check = not(x == 0 and y == 0);
                   auto const b : m_boid_subspaces[subspace_id + glm::vec2{x, y}])
                if ((skip_self_check or boid.id != b.id) and
                    glm::length(boid.position - b.position) <= m_view_distance + m_boid_width * 0.5f)
                  m_boid_neighbors.push_back(b);
          return std::tuple{m_boid_neighbors.size()};
        }();
        auto const [separation, alignment, cohesion] = [&]
        {
          if (m_boid_neighbors.empty()) return std::tuple{glm::vec2{}, glm::vec2{}, glm::vec2{}};
          auto total_separation = glm::vec2{};
          auto total_velocity   = glm::vec2{};
          auto total_position   = glm::vec2{};
          for (auto const &b : m_boid_neighbors)
          {
            auto const gap       = boid.position - b.position;
            auto const distance  = glm::length(gap);
            total_separation    += gap / (distance * distance * distance);
            total_velocity      += b.velocity;
            total_position      += b.position;
          }
          auto const average_separation = total_separation / static_cast<float>(m_boid_neighbors.size());
          auto const average_velocity   = total_velocity / static_cast<float>(m_boid_neighbors.size());
          auto const average_position   = total_position / static_cast<float>(m_boid_neighbors.size());
          auto       separation         = average_separation;
          auto       alignment          = average_velocity - boid.velocity;
          auto       cohesion           = average_position - boid.position;
          for (auto *const f : {&separation, &alignment, &cohesion})
          {
            auto &force = *f;
            if (glm::length(force) < -m_max_acceleration) force *= -m_max_acceleration / glm::length(force);
            if (glm::length(force) > +m_max_acceleration) force *= +m_max_acceleration / glm::length(force);
          }
          return std::tuple{separation, alignment, cohesion};
        }();
        auto const [mouse_flee] = [&]
        {
          auto mouse_gap  = boid.position - mouse_pos;
          auto mouse_flee = glm::vec2{};
          if (glm::length(mouse_gap) < 0.1f) mouse_flee = glm::normalize(mouse_gap) * 10.0f;
          return std::tuple{mouse_flee};
        }();

        boid.acceleration += m_weight_separation /* */ * separation /* */ +
                             m_weight_alignment /*  */ * alignment /*  */ +
                             m_weight_cohesion /*   */ * cohesion /*   */ +
                             mouse_flee;
        if (glm::length(boid.acceleration) < m_min_acceleration) boid.acceleration *= m_min_acceleration / glm::length(boid.acceleration);
        if (glm::length(boid.acceleration) > m_max_acceleration) boid.acceleration *= m_max_acceleration / glm::length(boid.acceleration);
        boid.velocity += boid.acceleration * dt;
        if (glm::length(boid.velocity) < m_min_velocity) boid.velocity *= m_min_velocity / glm::length(boid.velocity);
        if (glm::length(boid.velocity) > m_max_velocity) boid.velocity *= m_max_velocity / glm::length(boid.velocity);
        boid.position += boid.velocity * dt;
        if (glm::abs(boid.position.x) > 1.0f + m_boid_width) boid.position.x = glm::clamp(-boid.position.x, -1.0f - m_boid_width, 1.0f + m_boid_width);
        if (glm::abs(boid.position.y) > 1.0f + m_boid_width) boid.position.y = glm::clamp(-boid.position.y, -1.0f - m_boid_width, 1.0f + m_boid_width);
      }

      if constexpr (not g_using_emscripten) // debug display
      {
        auto duration                  = static_cast<float>(glfwGetTime()) - time;
        auto thread_local ave_duration = duration;
        ave_duration                   = (ave_duration * 99.0f + duration * 1.0f) / 100.0f;
        auto const lines               = std::array{
            std::pair{std::string_view{"  s/frames"}, static_cast<float>(ave_duration)},
            std::pair{std::string_view{"  frames/s"}, static_cast<float>(1.0f / ave_duration)},
            std::pair{std::string_view{"      tick"}, static_cast<float>(m_tick)},
            std::pair{std::string_view{" subspaces"}, static_cast<float>(m_boid_subspaces.size())},
        };
        auto buf_table = std::array<char, 32 * 2 * lines.size()>{};
        auto buf_float = std::array<char, 16>{};
        auto table     = buf_table.data();
        table          = std::format_to(table, "\033[s");
        for (auto row = 1; auto const [title, value] : lines)
          table = std::format_to(table, "\033[{};80H\033[46m {:>10} \033[42m {:>10} \033[m", row++, title, //
                                 std::string_view{buf_float.data(), static_cast<size_t>(std::format_to(buf_float.data(), "{:4.5f}", value) - buf_float.data())});
        table = std::format_to(table, "\033[u");
        std::print("{}", std::string_view{buf_table.data(), static_cast<size_t>(table - buf_table.data())});
      }
    }
    auto render() -> void override
    {
      glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
      if (m_render_tick != m_tick)
      {
        m_render_tick   = m_tick;
        auto const data = std::as_bytes(std::span{m_boids});
        if (m_vbo_size < data.size() or m_vbo_size > data.size() * 2zu)
          glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(m_vbo_size = data.size()), nullptr, GL_DYNAMIC_DRAW), glCheckError();
        glBufferSubData(GL_ARRAY_BUFFER, /* offset */ 0, static_cast<GLsizeiptr>(data.size()), data.data()), glCheckError();
      }

      glUseProgram(m_pid);
      glUniform1f(m_uniforms.boid_width, m_boid_width);
      glCheckError();

      glBindVertexArray(m_vao);
      glDrawArraysInstanced(GL_TRIANGLE_STRIP, /* vertex index offset */ 0, /* quad vertex count */ 3, static_cast<GLsizei>(m_boids.size()));
      glCheckError();
    }

  private:
    using m_subspaces_hashmap_t = std::unordered_map<
        glm::i32vec2, std::vector<boid>,
        decltype([]<typename T>(glm::vec<2, T> const p)
                 { return std::hash<T>{}(p.x) xor
                          std::hash<T>{}(p.y); })>;
    struct m_uniforms_t
    {
        int32_t boid_width{};
    };
    uint32_t              m_pid{}, m_vid{}, m_fid{}, m_vbo{}, m_vao{};
    size_t                m_vbo_size{}, m_tick{}, m_render_tick{};
    m_uniforms_t          m_uniforms{};
    glm::float32_t        m_time{};
    std::vector<boid>     m_boids{}, m_boid_neighbors{};
    m_subspaces_hashmap_t m_boid_subspaces{};

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

  private:
    float m_min_position      = -1.0f,
          m_max_position      = +1.0f,
          m_min_velocity      = +0.0f,
          m_max_velocity      = +10.0f,
          m_min_acceleration  = +0.1f,
          m_max_acceleration  = +0.4f;
    float m_view_distance     = +0.1f,
          m_boid_width        = m_view_distance / 4.0f,
          m_subspace_width    = m_view_distance / 2.0f;
    float m_weight_separation = +0.11f,
          m_weight_alignment  = +1.9f,
          m_weight_cohesion   = +1.3f;
    size_t m_tick_rate        = 60zu,
           m_boid_count       = 800zu,
           m_subspace_count   = static_cast<size_t>(std::ceil((m_max_position - m_min_position) / m_subspace_width));
};

/*================================*\
|*================================*|
\*================================*/

int main()
{
  auto app = application{};
  app.schedule_layer_push<layers::clear>(-1, glm::vec4{0.15f, 0.05f, 0.5f, 1.0f});
  app.schedule_layer_push<layers::boids>();
  return app.run();
}

/*================================*\
|*================================*|
\*================================*/
