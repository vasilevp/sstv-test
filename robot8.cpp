#include "robot8.hpp"

#include <stdexcept>
#include <cstdint>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

void Robot8::Encode()
{
    if ((height % 120) != 0)
    {
        throw std::runtime_error("Image height must be divisible by 120!");
    }

    writeHeader();

    const float pixelTime = lineTime / width;
    const int stride = height / 120;

    for (size_t i = 0; i < height; i += stride)
    {
        // sync pulse
        s.synth(syncTime, 1200);

        for (size_t j = 0; j < width; ++j)
        {
            size_t offset = (i * width + j) * 3;
            float Y = getY(pixels, offset);
            int freq = 1500 + 800 * Y / 255;

            // pixel
            s.synth(pixelTime, freq);
        }
    }
}

void Robot8::WriteGreyscale(int size)
{
    const float syncTime = 5;
    const float pixelTime = 56.0f / width;

    for (size_t i = 0; i < 8; ++i)
    {
        // sync pulse
        s.synth(syncTime, 1200);

        for (size_t j = 0; j < width; ++j)
        {
            size_t offset = (i * width + j) * 3;
            float val = float(j * 255) / width;

            int freq = 1500 + 800 * val / 255;

            // pixel
            s.synth(pixelTime, freq);
        }
    }
}