#pragma once

#include <iostream>

namespace Logger
{
  inline void info(const char *data)
  {
    std::cout << data << std::endl;
  }
  inline void info(std::string data)
  {
    std::cout << data << std::endl;
  }
} // namespace Logger
