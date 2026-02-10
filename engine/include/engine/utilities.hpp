#ifndef ENGINE_UTILITIES_HPP
#define ENGINE_UTILITIES_HPP

#include <engine/core.hpp>

namespace engine::utilities
{
  struct runtime_assert_failure : std::runtime_error
  {
      using std::runtime_error::runtime_error;
  };
  template <typename exception = runtime_assert_failure, typename T, typename... Args>
    requires(std::constructible_from<exception, std::string> and std::constructible_from<bool, T>)
  auto inline /*     */ runtime_assert(T &&value, std::format_string<Args...> message_format = "failed runtime assert", Args &&...message_args) -> T
  {
    if (static_cast<bool>(std::forward<T>(value))) [[likely]]
      return std::forward<T>(value);
    auto message = std::format(message_format, std::forward<Args>(message_args)...);
    throw exception{std::move(message)};
  }

  template <typename value_t, typename compare_t = std::ranges::less, typename project_t = std::identity>
    requires std::sortable<std::ranges::iterator_t<std::span<value_t>>, compare_t, project_t>
  auto /*  */ constexpr heap_sort_partial(std::span<value_t> const range, size_t const n = std::numeric_limits<size_t>::max(), compare_t compare = {}, project_t project = {}) -> auto /* [unsorted, sorted] */
  {
    auto const sorted_size   = std::min(range.size(), n),
               unsorted_size = range.size() - sorted_size;
    if (sorted_size > 0zu)
      std::ranges::make_heap(range, compare, project);
    for (auto const i : std::views::iota(0zu, sorted_size))
      std::ranges::pop_heap(range.subspan(0zu, range.size() - i), compare, project);
    return std::tuple{range.subspan(0zu, unsorted_size),
                      range.subspan(/**/ unsorted_size)};
  }

  template <typename T>
  auto inline constexpr static_cast_lambda = []<typename U>(U &&value) static
    requires std::constructible_from<T, U>
  { return static_cast<T>(std::forward<U>(value)); };

  struct print_table_column_t
  {
      template <typename T>
        requires(std::constructible_from<std::string_view, T> or std::integral<std::remove_cvref_t<T>> or std::floating_point<std::remove_cvref_t<T>>)
      /**/ inline constexpr print_table_column_t(T &&value) noexcept
      {
        /**/ if constexpr (std::constructible_from<std::string_view, T>)
          variant.emplace<std::string_view>(std::forward<T>(value));
        else if constexpr (std::signed_integral<std::remove_cvref_t<T>>)
          variant.emplace<intmax_t>(std::forward<T>(value));
        else if constexpr (std::unsigned_integral<std::remove_cvref_t<T>>)
          variant.emplace<uintmax_t>(std::forward<T>(value));
        else if constexpr (std::floating_point<std::remove_cvref_t<T>>)
          variant.emplace<double_t>(std::forward<T>(value));
        else
          static_assert(false);
      }
      std::variant<std::string_view, intmax_t, uintmax_t, double_t> variant = "";
  };
  auto inline constexpr print_table_lines_inline_buffer_count = 16zu;
  auto /*  */ /*     */ print_table_from_spans(std::span<std::span<print_table_column_t const> const> const lines) -> void;
  auto /*  */ /*     */ print_table(std::initializer_list<std::initializer_list<print_table_column_t>> lines) -> void;

  auto /*  */ /*     */ read_all(char const *const /*      */ file_path, char const *const mode = "r") -> std::expected<std::string, std::error_code>;
  auto inline /*     */ read_all(std::filesystem::path const &file_path, char const *const mode = "r") -> decltype(read_all(file_path.string().c_str(), mode))
  {
    if constexpr (std::convertible_to<decltype(file_path.c_str()), char const *>)
      return read_all(file_path.c_str(), mode);
    else
      return read_all(file_path.string().c_str(), mode);
  }

} // namespace engine::utilities
namespace engine { using utilities::runtime_assert; }

#endif // ENGINE_UTILITIES_HPP
