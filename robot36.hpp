#pragma once
#include <string>

#include "encoder.hpp"

class Robot : Encoder
{
public:
    Robot(const std::string &input, const std::string &output) : Encoder(input, output, vCode()) {};
    void Encode();

private:
    constexpr uint8_t vCode() { return 8; };
    void WriteGreyscale();

    const float syncPulse = 9;
    const float syncPorch = 3;
    const float lineTime = 88;
    const int greyscaleLines = 16;
};
