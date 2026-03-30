#ifndef ENGINE_UTILITIES_HPP
#define ENGINE_UTILITIES_HPP

#include <engine/core.hpp>

namespace engine::utilities /* runtime assert     */
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
} // namespace engine::utilities
namespace engine::utilities /* sorting            */
{
  template <typename value_t, typename compare_t = std::ranges::less, typename project_t = std::identity>
    requires(std::sortable<std::ranges::iterator_t<std::span<value_t>>, compare_t, project_t>)
  auto inline constexpr heap_sort_partial(std::span<value_t> const range, size_t const n = std::numeric_limits<size_t>::max(), compare_t compare = {}, project_t project = {}) -> auto /* [unsorted, sorted] */
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

  template <
      typename range_t,
      typename project_t = std::identity,
      typename value_t   = std::ranges::range_value_t<range_t>,
      typename sort_t    = std::decay_t<std::invoke_result_t<project_t, value_t const &>>,
      typename uint_t    = std::make_unsigned_t<std::tuple_element_t<std::countr_zero(sizeof(sort_t)), std::tuple<int8_t, int16_t, int32_t, int64_t>>>>
    requires(
        std::ranges::contiguous_range<range_t> and
        std::invocable<project_t, value_t const &> and
        std::integral<sort_t> and /*                                               */ (std::same_as<value_t, std::ranges::range_value_t<range_t>>) and
        sizeof(sort_t) == sizeof(uint_t) and std::unsigned_integral<uint_t> and /* */ (std::same_as<sort_t, std::decay_t<std::invoke_result_t<project_t, value_t const &>>>))
  auto inline /*     */ radix_sort(range_t &&range, project_t project = {}, size_t bits_per_pass = 8zu) -> range_t
  {
    if (std::ranges::size(range) <= 1zu) return std::forward<range_t>(range);
    auto const n_total_bits = sizeof(sort_t) * 8zu;
    auto const n_bits       = std::gcd(n_total_bits, std::max(4zu, std::min({32zu, n_total_bits, bits_per_pass})));
    auto const n_counts     = 1zu << n_bits;
    auto const n_passes     = (n_total_bits + n_bits - 1zu) / n_bits;
    auto const radix_mask   = (n_total_bits == n_bits) ? ~uint_t{0u} : uint_t{(1zu << n_bits) - 1zu};
    auto const values       = std::span<value_t>{range};
    auto /* */ alloc        = std::vector<size_t>(n_counts + values.size() * 2zu);
    auto const counts       = std::span{alloc}.subspan(0zu /*           */ /*                */, n_counts);
    auto /* */ old_indices  = std::span{alloc}.subspan(0zu + counts.size() /*                */, values.size());
    auto /* */ new_indices  = std::span{alloc}.subspan(0zu + counts.size() + old_indices.size(), values.size());
    for (auto const &i : std::views::iota(0zu, values.size())) new_indices[i] = i;
    for (auto const &i : std::views::iota(0zu, n_passes))
    {
      std::swap(old_indices, new_indices);
      auto const get_count /*         */ = [&](size_t index) -> size_t &
      {
        auto sort  = std::invoke(project, values[index]);
        auto uint  = static_cast<uint_t>(sort);
        auto radix = (uint >> (i * n_bits)) & (radix_mask);
        return counts[radix];
      };
      auto const counts_sign_order /* */ = [&]
      {
        auto const start_halfway = (std::signed_integral<sort_t>) and (i == n_passes - 1zu);
        auto const halfway       = counts.size() / 2zu;
        return //
            std::array{
                counts.subspan(start_halfway ? halfway : 0zu, halfway),
                counts.subspan(start_halfway ? 0zu : halfway, halfway),
            } |
            std::views::join;
      }();
      for (/*         */ auto /* */ &count : counts /*      */) count = 0zu;
      for (/*         */ auto const &index : old_indices /* */) (void /**/)(get_count(index)++);
      for (auto j = 0zu; auto /* */ &count : counts_sign_order) count = std::exchange(j, j + count);
      for (/*         */ auto const &index : old_indices /* */) new_indices[get_count(index)++] = index;
    }
    auto const target_indices = old_indices;
    auto const sorted_indices = new_indices;
    for (auto const &i : std::views::iota(0zu, values.size())) target_indices[sorted_indices[i]] = i;
    for (auto const &i : std::views::iota(0zu, values.size()))
    {
      while (i != target_indices[i])
      {
        std::swap(values /*   */[i], values /*   */[target_indices[i]]);
        std::swap(target_indices[i], target_indices[target_indices[i]]);
      }
    }
    return std::forward<range_t>(range);
  }
} // namespace engine::utilities
namespace engine::utilities /* static cast lambda */
{
  template <typename T>
  auto inline constexpr static_cast_lambda = []<typename U>(U &&value) static
    requires std::constructible_from<T, U>
  { return static_cast<T>(std::forward<U>(value)); };
} // namespace engine::utilities
namespace engine::utilities /* print ansi table   */
{
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
  auto /*  */ /*     */ print_ansi_table_from_spans(std::span<std::span<print_table_column_t const> const> const lines) -> void;
  auto /*  */ /*     */ print_ansi_table(std::initializer_list<std::initializer_list<print_table_column_t>> lines) -> void;
} // namespace engine::utilities
namespace engine::utilities /* read all form file */
{
  auto /*  */ /*     */ read_all(char const *const /*      */ file_path, char const *const mode = "r") -> std::expected<std::string, std::error_code>;
  auto inline /*     */ read_all(std::filesystem::path const &file_path, char const *const mode = "r") -> decltype(read_all(file_path.string().c_str(), mode))
  {
    if constexpr (std::convertible_to<decltype(file_path.c_str()), char const *>)
      return read_all(file_path.c_str(), mode);
    else
      return read_all(file_path.string().c_str(), mode);
  }
} // namespace engine::utilities
namespace engine::utilities /* fnv1a_hash         */
{
  auto inline static constexpr fnv1a_hash(std::string_view str, uint64_t basis = 0xcb'f2'9c'e4'84'22'23'25, uint64_t prime = 0x00'00'01'00'00'00'01'b3) noexcept
  {
    auto hash = basis;
    for (uint64_t chr : str) hash = (hash xor chr) * prime;
    return hash;
  }
} // namespace engine::utilities
namespace engine { using utilities::runtime_assert; }

#endif // ENGINE_UTILITIES_HPP
