# sstv-test
A Robot 8 B/W SSTV encoder made for shits and giggles.

## How to build and run
```
git clone git@github.com:exploser/sstv-test.git
cd sstv-test
git submodule update --init --recursive
cmake -S . -B build
cmake --build build -j
mkdir -p outputs
./build/encoder <input.bmp>
```

The build defaults to `Debug` (AddressSanitizer enabled). For an optimised
LTO build use `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`.
