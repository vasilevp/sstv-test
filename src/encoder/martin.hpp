#pragma once
#include <cstdint>
#include <stdexcept>
#include <string>

#include "encoder.hpp"

class Martin : Encoder
{
public:
	Martin(PixelSource &source,
		   Synthesizer &&s,
		   uint8_t mode,
		   const std::string &greeting = "")
		: Encoder(source, std::move(s), vCode(mode)),
		  mode(mode),
		  greeting(greeting)
	{
		utils::Guard();
	};
	void Encode();

private:
	const float syncPulse = 4.862;
	const float syncPorch = 0.572;
	const float lineTime = 73.216f;
	const uint8_t mode;
	const std::string &greeting;

	// VIS code per the SSTV handbook (Bruchanov, ch. 5).
	inline static constexpr uint8_t vCode(const uint8_t mode)
	{
		utils::Guard();

		switch (mode)
		{
		case 1: return 44;
		case 2: return 40;
		case 3: return 36;
		case 4: return 32;
		default:
			throw new std::invalid_argument("unknown mode");
			return 40;
		}
	};

	// Per-channel pixel-time multiplier: M1/M3 send each channel for
	// twice the base lineTime; M2/M4 send for one. The line timing of M3
	// matches M1, and M4 matches M2 — they differ only in line count.
	inline static constexpr uint8_t channelMultiplier(const uint8_t mode)
	{
		return (mode == 1 || mode == 3) ? 2 : 1;
	};

	// Standard image-line count for each Martin mode (handbook ch. 5).
	inline static constexpr uint32_t standardLines(const uint8_t mode)
	{
		return (mode <= 2) ? 256 : 128;
	};

	void writeGreeting();
	void colorLine(uint32_t i, size_t color);
};
