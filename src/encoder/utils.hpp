#pragma once
#include <array>
#include <cstdint>
#include <cstdlib>
#include <memory>

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define DEFER(x) std::shared_ptr<void> CONCAT(_defer___, __COUNTER__)(nullptr, [&](auto) { x; });

namespace utils
{
	static constexpr auto lut = std::to_array<int8_t>({
#include "sine2000.csv"
	});

	static constexpr bool getText(size_t i, size_t j, size_t size, const std::string &text)
	{
		// this is bullshit but it works
		// constexpr unsigned char font8x8_basic[128][8]
		constexpr unsigned
#include <font8x8/font8x8_basic.h>
			;

		auto character = j / 8 / size - 1;
		if (character < 0 || character >= text.size())
		{
			return false;
		}
		if (i / size >= 8)
		{
			return false;
		}

		auto glyph = font8x8_basic[text[character]];
		auto set = glyph[i / size % 8] & 1 << (j / size % 8);
		return set;
	}

}
