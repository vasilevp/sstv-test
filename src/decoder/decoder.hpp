#pragma once

#include <cstdint>
#include <vector>

class Decoder
{
public:
	~Decoder();

	virtual void Decode() = 0;

protected:
	Decoder(const std::vector<uint8_t> input);
};