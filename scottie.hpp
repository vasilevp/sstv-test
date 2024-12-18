#pragma once

#include "encoder.hpp"

class Synthesizer;
class Scottie : Encoder
{
public:
	enum Mode
	{
		S1 = 60,
		S2 = 56,
		DX = 76,
	};

	Scottie(const std::string &input, const Synthesizer &s, Mode mode, const std::string &greeting = "");

	void Encode();

private:
	void writeGreeting();

	const std::string &greeting;
	const float syncTime = 9;
	float lineTime = 88.064;
	void colorLine(int i, size_t color);
};
