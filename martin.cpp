#include "martin.hpp"

#include <stdexcept>
#include <cstdint>
#include <iostream>
#include <print>

#include "utils.hpp"

#include "LoadBMP/loadbmp.h"

using namespace std;

void Martin::Encode()
{
    if (height != 256)
    {
        print(cerr, "WARN: Image height must be 256 pixels! Provided height of {} may cause issues with the receiver.", height);
    }

    writeHeader();
    const float syncPulse = 4.862;
    const float syncPorch = 0.572;
    const float pixelTime = 73.216f * (3 - mode) / width;

    auto colorLine = [&](int i, size_t color) constexpr
    {
        // sync porch
        s.synth(syncPorch, 1500);

        for (size_t j = 0; j < width; ++j)
        {
            size_t offset = (i * width + j) * 3;
            float G = pixels[offset + color];

            int freq = 1500 + 800 * G / 255;

            // pixel
            s.synth(pixelTime, freq);
        }
    };

    for (size_t i = 0; i < height; ++i)
    {
        // sync pulse
        s.synth(syncPulse, 1200);

        colorLine(i, 1);
        colorLine(i, 2);
        colorLine(i, 0);

        // separator
        s.synth(syncPorch, 1500);
    }

    for (size_t i = height; i < 256; ++i)
    {
        // sync pulse
        s.synth(syncPulse, 1200);

        colorLine(height - 1, 1);
        colorLine(height - 1, 2);
        colorLine(height - 1, 0);

        // separator
        s.synth(syncPorch, 1500);
    }
}
