#pragma once

#include <fstream>
#include <type_traits>

std::vector<char> readFile(const std::string& path) {
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

inline constexpr uint32_t computeMipLevels(int width, int height) {
  // calculate number of times we can halve the image
  // add one for the base level 0
  return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) +
         1;
}

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