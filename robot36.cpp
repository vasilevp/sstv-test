#include "robot36.hpp"

#include <stdexcept>
#include <cstdint>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

void Robot::Encode()
{
    if (height != 240)
    {
        throw std::runtime_error("Image height must be 240 pixels!");
    }

    writeHeader();
    const float pixelTime = lineTime / width;

    WriteGreyscale();

    for (size_t i = 0; i < height; ++i)
    {
        // sync pulse
        s.synth(syncPulse, 1200);
        // sync porch
        s.synth(syncPorch, 1500);

        for (size_t j = 0; j < width; ++j)
        {
            size_t offset = (i * width + j) * 3;
            float Y = getY(pixels, offset);

            int freq = 1500 + 800 * Y / 255;

            // pixel
            s.synth(pixelTime, freq);
        }

        bool odd = i % 2;
        auto getColor = (odd ? getBY : getRY);

        // even line separator
        s.synth(syncPulse / 2, odd ? 2300 : 1500);
        // porch
        s.synth(syncPorch / 2, 1900);

        for (size_t j = 0; j < width; ++j)
        {
            size_t offset = (i * width + j) * 3;
            float Y = getColor(pixels, offset);
            float Y2 = getColor(pixels, ((i + 1) * width + j) * 3);

            int freq = 1500 + 800 * (Y + Y2) / 2 / 255;

            // pixel
            s.synth(pixelTime / 2, freq);
        }
    }
}

void Robot::WriteGreyscale()
{
    const float pixelTime = lineTime / width;

    for (size_t i = 0; i < greyscaleLines; ++i)
    {
        // sync pulse
        s.synth(syncPulse, 1200);
        // sync porch
        s.synth(syncPorch, 1500);
        // line
        for (size_t j = 0; j < width; ++j)
        {
            float Y = float(j * 255) / width;
            int freq = 1500 + 800 * Y / 255;

            // pixel
            s.synth(pixelTime, freq);
        }

        // even line separator
        s.synth(syncPulse / 2, i % 2 == 0 ? 1900 : 2300);
        // sync porch
        s.synth(syncPorch / 2, 1900);
        // line
        s.synth(pixelTime / 2 * width, 1900);
    }
}