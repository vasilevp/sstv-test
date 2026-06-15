#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

// One full cycle of sin() sampled across 2000 phases and quantised to
// int8_t. The Synthesizer reads from this table per audio sample via
// direct integer-phase indexing into utils::lut (bit-exact, modular).
// The IQ demodulator's NCO indexes the same table via utils::sin /
// utils::cos with a float-radian argument.
//
// The table is computed at compile time via constexpr `<cmath>`
// (C++26, P1383R2). 127.5 maps sin's [-1, 1] range across the full
// int8_t band; std::floor (round toward -∞) then yields the standard
// signed 8-bit quantisation [-127, 127], matching the offline CSV the
// project used to ship alongside this header.

namespace utils::detail
{
	constexpr float TwoPi = 2.0f * std::numbers::pi_v<float>;
	constexpr std::size_t SineLutSize = 2000;

	constexpr std::array<std::int8_t, SineLutSize> makeSineLut()
	{
		std::array<std::int8_t, SineLutSize> a{};
		for (std::size_t i = 0; i < SineLutSize; ++i)
		{
			const double t = 2.0 * std::numbers::pi * double(i) / double(SineLutSize);
			a[i] = static_cast<std::int8_t>(std::floor(127.5 * std::sin(t)));
		}
		return a;
	}

	// Map any radians value to its [0, SineLutSize) table index. Small
	// while-loops handle wrap rather than std::fmod because typical NCO
	// callers already keep their phase in [0, 2π) and the loops then
	// become no-ops.
	constexpr std::uint32_t phaseToLutIdx(float radians)
	{
		while (radians <  0.0f)  radians += TwoPi;
		while (radians >= TwoPi) radians -= TwoPi;
		return std::uint32_t(radians * float(SineLutSize) / TwoPi) % SineLutSize;
	}
}

namespace utils
{
	constexpr auto lut = detail::makeSineLut();

	// Drop-in std::sin / std::cos replacements backed by the int8 LUT.
	// 8-bit amplitude quantisation (~48 dB SNR) — plenty of headroom for
	// the IQ demodulator's NCO mixer; not for serious DSP elsewhere.
	constexpr float sin(float radians)
	{
		return float(lut[detail::phaseToLutIdx(radians)]) * (1.0f / 127.0f);
	}

	constexpr float cos(float radians)
	{
		// cos(x) = sin(x + π/2) — index-offset by a quarter table.
		constexpr std::uint32_t QuarterTable = detail::SineLutSize / 4;
		const std::uint32_t idx = (detail::phaseToLutIdx(radians) + QuarterTable)
		                         % detail::SineLutSize;
		return float(lut[idx]) * (1.0f / 127.0f);
	}
}
