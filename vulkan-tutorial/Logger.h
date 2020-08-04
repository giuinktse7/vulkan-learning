#pragma once

#include <iostream>
#include <string_view>

constexpr std::string_view InfoString = "[INFO]";

#define LOG_INFO std::cout << info

namespace Logger
{
  std::ostream &info();
} // namespace Logger
