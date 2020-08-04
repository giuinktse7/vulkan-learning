#include "logger.h"

std::ostream &Logger::info()
{
    std::cout << InfoString;
    return std::cout;
}
