#pragma once

#include "encoder.hpp"

class Synthesizer;
class Scottie : Encoder
{
public:
	Scottie(RowSource &source, Synthesizer &&s, sstv::VisCode mode, const std::string &greeting = "");

	void Encode();

private:
	void writeGreeting();

	const std::string &greeting;
	const float syncTime = 9;
	float lineTime = 88.064;
	// Image-line count per the SSTV handbook: 256 for S1/S2/DX, 128 for S3/S4.
	uint32_t standardLines = 256;
	void colorLine(uint32_t i, size_t color);
};
