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
    if (static_cast<bool>(std::forward<T>(value))) return std::forward<T>(value);
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
  auto /*  */ /*     */ print_table(std::span<std::tuple<std::string_view, double> const> const &lines) -> void;
  auto inline /*     */ print_table(std::initializer_list<std::tuple<std::string_view, double>> const &lines) -> void { return print_table(std::span{lines.begin(), lines.size()}); }
  auto /*  */ /*     */ read_all(char const *const file_path, char const *const mode = "r") -> std::expected<std::string, std::error_code>;

} // namespace engine::utilities
namespace engine { using utilities::runtime_assert; }

#endif // ENGINE_UTILITIES_HPP
