#pragma once
#include <string>
#include <stdexcept>

#include "encoder.hpp"

class Martin : Encoder
{
public:
	Martin(const std::string &input,
		   Synthesizer &&s,
		   uint8_t mode,
		   const std::string &greeting = "")
		: Encoder(input, s, vCode(mode)),
		  mode(mode),
		  greeting(greeting) {};
	void Encode();

private:
	const float syncPulse = 4.862;
	const float syncPorch = 0.572;
	const float lineTime = 73.216f;
	const uint8_t mode;
	const std::string &greeting;
	constexpr uint8_t vCode(const uint8_t mode) const
	{
		switch (mode)
		{
		case 1:
			return 44;
		case 2:
			return 40;
		default:
			throw new std::invalid_argument("unknown mode");
			return 40;
		}
	};

	void writeGreeting();
	void colorLine(int i, size_t color);
};
