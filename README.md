# sstv-test
A Robot 8 B/W SSTV encoder made for shits and giggles.

## How to build and run
```
git clone git@github.com:exploser/sstv-test.git
git submodule update --init --recursive
autoreconf -i
./configure
make -j$(nproc)
./sstv
```
