#pragma once

#include <type_traits>
#include <variant>
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

template <typename E>
constexpr auto to_underlying(E e) noexcept
{
	return static_cast<std::underlying_type_t<E>>(e);
}

namespace util
{
	template <typename T>
	inline void appendVector(std::vector<T> &&source, std::vector<T> &destination)
	{
		if (destination.empty())
			destination = std::move(source);
		else
			destination.insert(std::end(destination),
												 std::make_move_iterator(std::begin(source)),
												 std::make_move_iterator(std::end(source)));
	}

	template <typename T, typename... Args>
	struct is_one_of : std::disjunction<std::is_same<std::decay_t<T>, Args>...>
	{
	};

	// helper type for the visitor #4
	template <class... Ts>
	struct overloaded : Ts...
	{
		using Ts::operator()...;
	};
	// explicit deduction guide (not needed as of C++20)
	template <class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;
} // namespace util
