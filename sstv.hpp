#pragma once
#include <string>
#include <stdexcept>

#include "utils.hpp"
#include "wav.hpp"

#define LOADBMP_IMPLEMENTATION
#include "LoadBMP/loadbmp.h"

class SSTV
{
  public:
    static void Encode(const std::string &input, const std::string &output)
    {
#define SYNTH(length, freq)                                                      \
    {                                                                            \
        auto frame = ms2samp(length, sample_rate);                               \
        while (frame--)                                                          \
        {                                                                        \
            uint8_t x = utils::lut[((idx / freq_step) % utils::lut_size)] + 128; \
            idx += freq;                                                         \
            w.put(x);                                                            \
        }                                                                        \
    }

        // load BMP
        uint8_t *pixels = NULL;
        uint32_t width, height;
        auto err = loadbmp_decode_file(input.c_str(), &pixels, &width, &height, LOADBMP_RGB);
        DEFER(free(pixels));

        if (err)
        {
            throw std::runtime_error("Error " + std::to_string(err));
        }

        if (width != 160 || height != 120)
        {
            throw std::runtime_error("Image must be 160x120 pixels!");
        }

        // MUST BE A MULTIPLE OF 2000
        const size_t sample_rate = 44000;

        WAVWriter w(output, sample_rate);

        const int freq_step = sample_rate / utils::lut_size;

        int idx = 0;

        /*
            // sstv header
            // calibration pulse
            SYNTH(300, 1900);
            SYNTH(10, 1200);
            SYNTH(300, 1900);
        
            // VIS code (all zeros because documentation on this sucks balls)
            // start/stop marker
            SYNTH(30, 1200);
        
            // 7 zero bits
            for (int i = 0; i < 7; ++i)
            {
                // 1300 = 0, 1100 = 1
                SYNTH(30, 1300);
            }
        
            // parity bit
            SYNTH(30, 1300);
            */

        // start/stop marker
        SYNTH(30, 1200);

        const float time = 8020.f; // in ms
        const float sync_time = 5;
        const float pixel_time = (time / height - sync_time) / width;

        for (size_t i = 0; i < height; ++i)
        {
            // sync pulse
            SYNTH(sync_time, 1200);

            for (size_t j = 0; j < width; ++j)
            {
                size_t offset = (i * width + j) * 3;
                float val = float(pixels[offset] + pixels[offset + 1] + pixels[offset + 2]) / 3;

                int freq = 1500 + 800 * val / 255;

                // pixel
                SYNTH(pixel_time, freq);
            }
        }

#undef SYNTH
    }

  private:
    static size_t ms2samp(float ms, size_t sample_rate)
    {
        return sample_rate * ms / 1000;
    }
};
