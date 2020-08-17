#ifndef __UTIL_H_
#define __UTIL_H_
#include <optional>
#include <type_traits>
#include <fmt/format.h>
namespace util {
template<typename T> requires std::is_pointer<T>::value T inline logError(std::string_view str)
{
    fmt::print(stderr, "Error: {}\n", str);
    return nullptr;
}
}// namespace util
#endif// __UTIL_H_
