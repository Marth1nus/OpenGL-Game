#include <engine/utilities.hpp>

#if /* */ defined(ftell64) or defined(fseek64)
#error "Macro name collision"
#endif //  defined(ftell64) or defined(fseek64)

#if /* */ defined(_WIN32) or defined(_WIN64)
#define ftell64 ::_ftelli64
#define fseek64 ::_fseeki64
#else //  defined(_WIN32) or defined(_WIN64)
#define ftell64 ::ftello
#define fseek64 ::fseeko
#endif // defined(_WIN32) or defined(_WIN64)

auto engine::utilities::print_table_from_spans(std::span<std::span<print_table_column_t const> const> const lines) -> void
{
  auto str_buf = std::array<char, 0x01'00zu * print_table_lines_inline_buffer_count>{};
  auto str_mbr = std::pmr::monotonic_buffer_resource{str_buf.data(), str_buf.size()};
  auto str     = std::pmr::string{&str_mbr};
  auto col_buf = std::array<char, 16zu + 1zu>{};
  auto col_mbr = std::pmr::monotonic_buffer_resource{col_buf.data(), col_buf.size()};
  auto col     = std::pmr::string{&col_mbr};
  str.reserve(str_buf.size());
  col.reserve(col_buf.size());
  for (auto const &[i, columns] : lines | std::views::enumerate)
  {
    std::format_to(std::back_inserter(str), "\033[{};0H\033[999C\033[{}D", 1zu + i, columns.size() * 16zu + 4zu);
    for (auto const &[j, column] : columns | std::views::enumerate)
    {
      col.clear();
      std::visit(
          [&col]<typename T>(T const &value)
          {
            /**/ if constexpr (std::floating_point<T>)
              std::format_to(std::back_inserter(col), "{:7.6f}", value);
            else if constexpr (std::integral<T>)
              std::format_to(std::back_inserter(col), "{:7} {:6}", value, "");
            else
              std::format_to(std::back_inserter(col), "{}", value);
          },
          column);
      auto const color = std::array{6, 2}.at((columns.size() - j) % 2zu);
      std::format_to(std::back_inserter(str), "\033[4{}m {:>16} ", color, col);
    }
    std::format_to(std::back_inserter(str), "\033[m");
  }
  std::print("\033[s{}\033[u", str);
}
auto engine::utilities::print_table(std::initializer_list<std::initializer_list<print_table_column_t>> lines) -> void
{
  using lines_spans_value_t = std::span<print_table_column_t const>;
  alignas(lines_spans_value_t) auto
             lines_buf   = std::array<std::byte, sizeof(lines_spans_value_t) * print_table_lines_inline_buffer_count>{};
  auto       lines_mbr   = std::pmr::monotonic_buffer_resource{lines_buf.data(), lines_buf.size()};
  auto const lines_spans = std::pmr::vector<lines_spans_value_t>{std::from_range, lines | std::views::transform(static_cast_lambda<lines_spans_value_t>), &lines_mbr};
  print_table_from_spans(lines_spans);
}
auto engine::utilities::read_all(char const *const file_path, char const *const mode) -> std::expected<std::string, std::error_code>
{
  auto const file     = std::fopen(file_path, mode);
  auto const file_ptr = std::unique_ptr<std::FILE, decltype([](std::FILE *p) static
                                                            { return std::fclose(p); })>{file};
  auto       len      = decltype(ftell64(file)){};
  auto       str      = std::string{};
  if ((file_ptr) and
      (fseek64(file, 0, SEEK_END) == 0) and
      (len = ftell64(file),
       len >= 0) and
      (fseek64(file, 0, SEEK_SET) == 0) and
      (str.resize_and_overwrite(len, [file](char *buf, size_t len)
                                { return std::fread(buf, sizeof(char), len, file); }),
       str.size() == static_cast<size_t>(len)))
    return {std::move(str)};
  else
    return std::unexpected{std::error_code{errno, std::generic_category()}};
}