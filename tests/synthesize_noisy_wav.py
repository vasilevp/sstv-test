"""Synthesize noisy SSTV WAVs for testing decoder robustness.

Takes the clean encoder outputs and mixes in:
  - 60 Hz mains hum at ~half signal amplitude (severe out-of-band noise)
  - broadband white hiss
  - a 5 kHz whistle above the SSTV band

The hum and whistle live well outside the 1100..2300 Hz SSTV tone range,
so a well-designed input bandpass should reject them and let the
downstream demodulator decode the signal cleanly.

Produces noisy variants for both Martin M1 (a slow PAL-like raw-RGB mode)
and Robot 36 (a faster Y/Cr/Cb mode that alternates chroma channels), so
the quality table can compare cadence-lock behaviour on different sync
cadences.

Run from the project root after building the encoder:

    ./build/encoder colortest.bmp           # produces every outputs/*.wav
    python3 tests/synthesize_noisy_wav.py   # writes exp/noisy_*.wav
"""

import math
import os
import random


# (mode_name, hum_amp, hiss_amp, whistle_amp). sig_amp is fixed across
# modes — the encoder writes the signal at a constant level.
NOISY_MODES = [
    ('martin1', 30, 12, 15),
    ('robot36', 30, 12, 15),
]


def synthesize(mode_name, hum_amp, hiss_amp, whistle_amp):
    with open(f'outputs/{mode_name}.wav', 'rb') as f:
        data = f.read()

    header = data[:44]
    samples = bytearray(data[44:])
    n = len(samples)
    fs = 8000

    print(f'samples in {mode_name}.wav: {n:,}')

    random.seed(42)
    out = bytearray(n)
    for i in range(n):
        s = samples[i] - 128
        hum = hum_amp * math.sin(2 * math.pi * 60.0 * i / fs)
        hiss = hiss_amp * (random.random() * 2 - 1)
        whistle = whistle_amp * math.sin(2 * math.pi * 5000.0 * i / fs)
        v = s + hum + hiss + whistle
        v = max(-128, min(127, v))
        out[i] = int(v) + 128

    out_path = f'exp/noisy_{mode_name}.wav'
    with open(out_path, 'wb') as f:
        f.write(header)
        f.write(bytes(out))

    print(f'wrote {out_path}')


def main():
    os.makedirs('exp', exist_ok=True)
    for mode, hum, hiss, whistle in NOISY_MODES:
        synthesize(mode, hum, hiss, whistle)


if __name__ == '__main__':
    main()
