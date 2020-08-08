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
	TimeMeasure();

	template <typename T>
	static std::chrono::steady_clock::time_point addTime(std::chrono::steady_clock::time_point time, long long d)
	{

		auto rawTime = std::chrono::time_point_cast<T>(time).time_since_epoch().count() + d;
		std::chrono::time_point<std::chrono::steady_clock> result(T{rawTime});

		return result;
	}

	static long long diffMillis(std::chrono::steady_clock::time_point from, std::chrono::steady_clock::time_point to);
	static long long millis(std::chrono::steady_clock::time_point a);

	std::chrono::steady_clock::time_point startTime;

	long long elapsedMillis();
	long long elapsedMillis(std::chrono::steady_clock::time_point other);

	std::chrono::steady_clock::time_point ellapsedMillisAsTime();

	// Sets the start of the clock to the current time.
	void setStartNow();

	static TimeMeasure start();
	static std::chrono::steady_clock::time_point getCurrentTime();

private:
};

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}