#pragma once
#include <string>

#include "encoder.hpp"

class Robot8 : Encoder
{
public:
    Robot8(const std::string &input, const std::string &output) : Encoder(input, output, vCode()) {};
    void Encode();

private:
    constexpr uint8_t vCode() { return 1; };
    void WriteGreyscale(int size);

    const float syncTime = 5;
    const float lineTime = 56;
};
