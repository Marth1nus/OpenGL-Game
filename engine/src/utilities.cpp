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