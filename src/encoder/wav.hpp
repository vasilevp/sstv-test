#pragma once
#include <iostream>
#include <stacktrace>
#include <string>
#include <vector>

class WAVWriter
{
	size_t sample_rate;

	FILE *f = nullptr;

	std::vector<uint8_t> samples;

public:
	// Avoid copying because we own *f
	WAVWriter(const WAVWriter &) = delete;
	WAVWriter &operator=(const WAVWriter &) = delete;

	// Move constructor
	WAVWriter(WAVWriter &&other) noexcept;
	WAVWriter &operator=(WAVWriter &&other) = default;

	WAVWriter(const std::string &name, const size_t sample_rate);
	~WAVWriter();

	void put(uint8_t sample);
};
