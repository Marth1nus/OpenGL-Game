#ifndef ENGINE_UTILITIES_HPP
#define ENGINE_UTILITIES_HPP

#include <engine/core.hpp>

namespace engine::utilities
{
  struct runtime_assert_failure : std::runtime_error
  {
      using std::runtime_error::runtime_error;
  };
  template <std::constructible_from<std::string> exception = runtime_assert_failure, std::convertible_to<bool> T, typename... Args>
  auto inline /*     */ runtime_assert(T &&value, std::format_string<Args...> message_format = "failed runtime assert", Args &&...message_args) -> decltype(value)
  {
    if (value) return std::forward<decltype(value)>(value);
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
  template <typename... lines_t>
    requires(((std::tuple_size_v<lines_t> == 2zu) and ...) and (std::formattable<std::tuple_element_t<0, lines_t>, char> and ...) and (std::formattable<std::tuple_element_t<1, lines_t>, char> and ...))
  auto /*  */ /*     */ terminal_table(std::tuple<lines_t...> const &lines)
  {
    using namespace std::literals;
    auto static constexpr fmt           = "\033[{};0H\033[999C\033[36D\033[46m {:>16} \033[42m {:>16} \033[m"sv;
    auto static constexpr column_widths = []
    {
      auto widths = std::array<size_t, 2zu>{};
      for (auto i = 0zu; auto &width : widths)
      {
        auto const start = fmt.find(":>", i);
        auto const end   = start /* */ == fmt.npos ? fmt.npos : fmt.find("}", start + 1);
        auto const str   = end /*   */ == fmt.npos ? ""sv : fmt.substr(start, end - start);
        if (str.empty()) break;
        for (auto const c : str)
          if ('0' <= c and c <= '9')
            width = (width * 10zu + c - '0');
        i = end + 1;
      }
      return widths;
    }();
    auto static constexpr line_width = []
    {
      auto total_column_width = 0zu;
      for (auto const width : column_widths) total_column_width += width;
      return total_column_width += fmt.size();
    }();
    auto mbr_buf = std::array<char, line_width * sizeof...(lines_t) + std::size("\033[s (lines) \033[u (safety padding)")>{};
    auto mbr     = std::pmr::monotonic_buffer_resource{mbr_buf.data(), mbr_buf.size()};
    auto str     = std::pmr::string{&mbr};
    auto it      = std::back_inserter(str);
    auto row     = 1z;
    str.reserve(mbr_buf.size());
    std::format_to(it, "\033[s");
    auto const for_line = [&](auto const &line)
    {
      auto const &[title, data] = line;
      auto       num_buf        = std::array<char, 32zu>{};
      auto       num_mbr        = std::pmr::monotonic_buffer_resource{num_buf.data(), num_buf.size()};
      auto       num_str        = std::pmr::string{&num_mbr};
      auto const value          = [&] -> auto const &
      {
        if constexpr (std::is_arithmetic_v<std::decay_t<decltype(data)>>)
        {
          num_str.reserve(num_buf.size());
          std::format_to(std::back_inserter(num_str), "{:7.6f}", static_cast<double>(data));
          return num_str;
        }
        else
        {
          return data;
        }
      }();
      std::format_to(it, fmt, row++, title, value);
    };
    std::apply([&](auto const &...lines)
               { (for_line(lines), ...); },
               lines);
    std::format_to(it, "\033[u");
    std::print("{}", str);
  }

  auto /*  */ /*     */ read_all(char const *file_path) -> std::expected<std::string, std::error_code>;
} // namespace engine::utilities
namespace engine { using utilities::runtime_assert; }

#endif // ENGINE_UTILITIES_HPP
