#pragma once
#include <string>
#include <stdexcept>

#include "encoder.hpp"

class Martin : Encoder
{
public:
    Martin(const std::string &input, const std::string &output, uint8_t mode) : mode(mode), Encoder(input, output, vCode(mode)) {};
    void Encode();

private:
    const uint8_t mode;
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
};