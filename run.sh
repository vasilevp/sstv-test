#!/usr/bin/env bash
set -e
cmake --build build -j | tail -4 && ./build/encoder colortest.bmp >/dev/null
env -u SHELL nix-shell --pure -p python3 --run \
  "python3 tests/synthesize_noisy_wav.py" >/dev/null
rm -rf exp/schmitt_clean exp/schmitt_noisy exp/schmitt_noisy_cadence
mkdir -p exp/schmitt_clean exp/schmitt_noisy exp/schmitt_noisy_cadence

# Clean: every mode × demod × prefilter.
for m in robot8 robot36 robot72 martin1 martin2 martin3 martin4 \
         scottie1 scottie2 scottie3 scottie4 scottieDX \
         pd50 pd90 pd120 pd160 pd180 pd240 pd290; do
  for k in zc iq; do
    for ftxt in nopf prefilter; do
      f=""
      [ "$ftxt" = "prefilter" ] && f="--prefilter"
      ./build/decoder "outputs/$m.wav" "exp/schmitt_clean/${m}_${k}_${ftxt}.bmp" \
        "--demod=$k" $f | grep "Wrote" &
    done
  done
done
wait

# Noisy: each test recording, with and without --cadence-lock.
noisy_decode() {
  local mode="$1" out_dir="$2" cad_flag="$3"
  for combo in "zc nopf" "zc prefilter" "iq nopf" "iq prefilter"; do
    local k ftxt f
    k=$(echo "$combo" | awk '{print $1}')
    ftxt=$(echo "$combo" | awk '{print $2}')
    f=""
    [ "$ftxt" = "prefilter" ] && f="--prefilter"
    ./build/decoder "exp/noisy_${mode}.wav" "exp/${out_dir}/${mode}_${k}_${ftxt}.bmp" \
      "--demod=$k" $f $cad_flag > /dev/null &
  done
}
for mode in martin1 robot36; do
  noisy_decode "$mode" schmitt_noisy         ""
  noisy_decode "$mode" schmitt_noisy_cadence "--cadence-lock"
done
wait
echo "all decodes done"
