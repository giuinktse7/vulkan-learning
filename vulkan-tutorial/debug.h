
#include <string>
#include <stdexcept>
#include <cassert>
#include <iostream>

namespace debug
{
#define ABORT_PROGRAM(message)                                                                              \
  std::cout << "[ERROR] " << __FILE__ << ", line " << (unsigned)(__LINE__) << ": " << message << std::endl; \
  abort()

#ifdef _DEBUG
#define DEBUG_ASSERT(exp, msg) \
  do                           \
    if (!exp)                  \
    {                          \
      ABORT_PROGRAM(msg);      \
    }                          \
    else                       \
    {                          \
    }                          \
  while (false)
#else
#define DEBUG_ASSERT(exp, msg) \
  do                           \
  {                            \
  } while (0)
#endif
} // namespace debug