#pragma once

#include <string>
#include <vector>
#include <chrono>

constexpr bool hasBitSet(uint32_t flag, uint32_t flags)
{
	return (flags & flag) != 0;
}

std::vector<const char *> getRequiredExtensions();

void to_lower_str(std::string &source);

void to_upper_str(std::string &source);

std::string as_lower_str(std::string s);

struct TimeMeasure
{
	std::chrono::steady_clock::time_point startTime;

	long long elapsedMillis();

	static TimeMeasure start();

private:
	TimeMeasure();
};
