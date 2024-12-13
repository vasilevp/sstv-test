#include "encoder.hpp"

#include <stdexcept>

#include "LoadBMP/loadbmp.h"

using namespace std;

Encoder::Encoder(const string &input, const string &output, uint8_t visCode) : visCode(visCode), s(output)
{
    // load BMP
    auto err = loadbmp_decode_file(input.c_str(), &pixels, &width, &height, LOADBMP_RGB);

    if (err)
    {
        throw runtime_error("Error " + to_string(err));
    }
}

Encoder::~Encoder()
{
    free(pixels);
}

void Encoder::writeHeader()
{
    // sstv header
    // calibration pulse
    s.synth(300, 1900);
    s.synth(10, 1200);
    s.synth(300, 1900);

    // VIS code
    // start/stop marker
    s.synth(30, 1200);

    // 8 => 0b1000 => 0 0 0 1 0 0 0
    auto code = visCode;
    uint8_t parity = 0;
    for (auto i = 0; i < 7; i++)
    {
        const size_t freq = (code & 1) ? 1100 : 1300;
        s.synth(30, freq);

        parity ^= code;
        code >>= 1;
    }

    parity ^= code;
    code >>= 1;
    const size_t freq = (parity & 1) ? 1100 : 1300;

    // parity bit
    s.synth(30, freq);

    // start/stop marker
    s.synth(30, 1200);
}