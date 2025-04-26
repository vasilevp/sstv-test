#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>

#include "font8x8_basic.h"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define DEFER(x) std::shared_ptr<void> CONCAT(_defer___, __COUNTER__)(nullptr, [&](auto) { x; });

namespace utils
{
	static inline constexpr auto lut = std::to_array<int8_t>({
#include "sine.txt"
	});

	static inline constexpr bool getText(size_t i, size_t j, size_t size, const std::string &text) noexcept
	{
		auto character = j / 8 / size - 1;
		if (character < 0 || character >= text.size())
		{
			return 0;
		}
		if (i / size >= 8)
		{
			return 0;
		}

		auto glyph = font8x8_basic[text[character]];
		auto set = glyph[i / size % 8] & 1 << (j / size % 8);
		return set;
	}

}
