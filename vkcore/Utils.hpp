#pragma once

#include <concepts>
#include <fstream>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace spoony::utils {
template <typename T>
concept Enum = std::is_enum_v<T>;

inline constexpr bool hasFlags(std::integral auto value,
                               std::integral auto flagsToCheck) {
  return (value & flagsToCheck) == flagsToCheck;
}

template <std::integral T, Enum E>
inline constexpr bool hasFlags(T value, E flagsToCheck) {
  auto flagsValue = static_cast<std::underlying_type_t<E>>(flagsToCheck);
  return (value & flagsValue) == flagsValue;
}

inline constexpr bool hasBit(std::convertible_to<size_t> auto value,
                             std::integral auto index) {
  return value & (1 << index);
}

inline std::vector<char> readFile(const std::string& path) {
  std::ifstream file(path, std::ios::ate | std::ios::binary);
  if (!file.is_open()) {
    throw std::runtime_error("unable to open file");
  }

  auto filesize = file.tellg();
  std::vector<char> buffer(filesize);

  file.seekg(0);
  file.read(buffer.data(), filesize);
  file.close();

  return buffer;
}

template <std::ranges::range R>
auto to_vector(R&& r) {
  std::vector<std::ranges::range_value_t<R>> v;

  // if we can get a size, reserve that much
  if constexpr (requires { std::ranges::size(r); }) {
    v.reserve(std::ranges::size(r));
  }

  // push all the elements
  for (auto&& e : r) {
    v.push_back(static_cast<decltype(e)&&>(e));
  }

  return v;
}
}  // namespace spoony::utils
