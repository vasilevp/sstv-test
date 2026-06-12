"""Compare every decoded image against the original colortest.bmp source.

For each mode and each (demodulator, prefilter) combination, compute PSNR
and SSIM between the decoded IMAGE portion (greeting stripped) and the
corresponding crop / subsample of the original colortest.bmp. The
"original clean image" is colortest.bmp — the encoder's input — not
another decode.

Outputs Markdown tables for clean and noisy decodes.

The decoded BMPs are expected in exp/schmitt_clean/ (named
<mode>_<demod>_<filt>.bmp) and exp/schmitt_noisy/ (named <demod>_<filt>.bmp).
See the project README for the encoder/decoder commands that produce them.

Run from the project root via nix-shell (scikit-image isn't system-wide on
NixOS, but the build environment provides everything else):

    env -u SHELL nix-shell --pure \\
        -p 'python3.withPackages (ps: with ps; \\
            [ scikit-image numpy pillow ])' \\
        bash --run "python3 tests/quality_table.py"
"""

import os
import numpy as np
from PIL import Image
from skimage.metrics import peak_signal_noise_ratio, structural_similarity


# -- Reference preparation -------------------------------------------------

def to_luma_rgb(img):
    """BT.601 luma of an RGB array, returned as a 3-channel image so it
    can be compared head-to-head with B/W decodes (which set R=G=B=Y)."""
    rgb = img.astype(np.float64)
    y = 0.299 * rgb[..., 0] + 0.587 * rgb[..., 1] + 0.114 * rgb[..., 2]
    y = np.clip(y, 0, 255).astype(np.uint8)
    return np.stack([y, y, y], axis=-1)


# -- Mode descriptors ------------------------------------------------------

# (name, greeting_rows, image_rows_emitted, ref_slice, is_bw)
#   greeting_rows:      rows to skip at the top of the decoded BMP.
#   image_rows_emitted: nominal image rows after the greeting.
#   ref_slice(ref):     function returning the corresponding crop or
#                       subsample of colortest.bmp.
#   is_bw:              true → reference is converted to luma to match.
MODES = [
    ('robot8',     8, 120, lambda c: c[::2],   True),
    ('robot36',   16, 240, lambda c: c,        False),
    ('robot72',   16, 240, lambda c: c,        False),
    ('martin1',   16, 240, lambda c: c,        False),
    ('martin2',   16, 240, lambda c: c,        False),
    ('martin3',   16, 128, lambda c: c[:128],  False),
    ('martin4',   16, 128, lambda c: c[:128],  False),
    ('scottie1',  16, 240, lambda c: c,        False),
    ('scottie2',  16, 240, lambda c: c,        False),
    ('scottie3',  16, 128, lambda c: c[:128],  False),
    ('scottie4',  16, 128, lambda c: c[:128],  False),
    ('scottieDX', 16, 240, lambda c: c,        False),
    ('pd50',      16, 240, lambda c: c,        False),
    ('pd90',      16, 240, lambda c: c,        False),
    ('pd120',     16, 240, lambda c: c,        False),
    ('pd160',     16, 240, lambda c: c,        False),
    ('pd180',     16, 240, lambda c: c,        False),
    ('pd240',     16, 240, lambda c: c,        False),
    ('pd290',     16, 240, lambda c: c,        False),
]

# (demod_kind, filter_suffix, label)
COMBOS = [
    ('zc', 'nopf',      'ZC'),
    ('zc', 'prefilter', 'ZC+pf'),
    ('iq', 'nopf',      'IQ'),
    ('iq', 'prefilter', 'IQ+pf'),
]


def measure(decoded_path, mode_info, ref_color):
    _, greeting, expected_rows, ref_slice, is_bw = mode_info
    dec_full = np.asarray(Image.open(decoded_path).convert('RGB'))
    dec = dec_full[greeting:greeting + expected_rows]

    ref = ref_slice(ref_color)
    if is_bw:
        ref = to_luma_rgb(ref)

    # Truncate to the rows that exist in both, so over-counted noisy
    # decodes still get a meaningful number computed on the part that
    # corresponds to real image content.
    h = min(dec.shape[0], ref.shape[0])
    dec = dec[:h]
    ref = ref[:h]

    return {
        'decoded_total_rows': dec_full.shape[0],
        'compared_rows': h,
        'psnr': peak_signal_noise_ratio(ref, dec, data_range=255),
        'ssim': structural_similarity(ref, dec, channel_axis=2, data_range=255),
    }


def fmt_psnr(v):
    if v is None:
        return '   n/a '
    if v == float('inf'):
        return '   inf '
    return f'{v:6.2f} '


def fmt_ssim(v):
    if v is None:
        return '  n/a '
    return f'{v:5.3f} '


def main():
    ref_color = np.asarray(Image.open('colortest.bmp').convert('RGB'))
    h, w = ref_color.shape[0], ref_color.shape[1]
    print(f'Reference: colortest.bmp ({w}×{h} RGB)\n')

    # --- Clean decodes -----------------------------------------------------

    cleans = []
    for mi in MODES:
        name = mi[0]
        row = {'mode': name, 'cells': {}}
        for demod, filt, label in COMBOS:
            path = f'exp/schmitt_clean/{name}_{demod}_{filt}.bmp'
            if not os.path.exists(path):
                row['cells'][label] = None
                continue
            row['cells'][label] = measure(path, mi, ref_color)
        cleans.append(row)

    print('### Clean decodes — PSNR (dB; higher = closer to colortest)')
    print()
    print('| Mode      ' + ''.join(f'| {l:>6} ' for _, _, l in COMBOS) + '|')
    print('|-----------' + '|' + '|'.join('--------' for _ in COMBOS) + '|')
    for r in cleans:
        cells = ''.join('| ' + fmt_psnr(r['cells'][l]['psnr'] if r['cells'][l] else None)
                        for _, _, l in COMBOS)
        print(f"| {r['mode']:<10}{cells}|")

    print()
    print('### Clean decodes — SSIM (1.000 = identical)')
    print()
    print('| Mode      ' + ''.join(f'| {l:>5} ' for _, _, l in COMBOS) + '|')
    print('|-----------' + '|' + '|'.join('-------' for _ in COMBOS) + '|')
    for r in cleans:
        cells = ''.join('| ' + fmt_ssim(r['cells'][l]['ssim'] if r['cells'][l] else None)
                        for _, _, l in COMBOS)
        print(f"| {r['mode']:<10}{cells}|")

    # --- Noisy decodes (noisy_*.wav vs colortest) -------------------------

    # Mode-name → display label for the section heading.
    noisy_modes = [
        ('martin1', 'Martin M1'),
        ('robot36', 'Robot 36'),
    ]
    noisy_dirs = [
        ('exp/schmitt_noisy',         'off'),
        ('exp/schmitt_noisy_cadence', 'on'),
    ]
    for mode_name, mode_label in noisy_modes:
        info = next(m for m in MODES if m[0] == mode_name)
        print()
        print(f'### Noisy {mode_name} decode vs colortest (mode = {mode_label})')
        print()
        print('Each (demod, prefilter) combination decoded with and without --cadence-lock.')
        print()
        print('| Combination          | cadence-lock | Total rows | PSNR (dB) |  SSIM  |')
        print('|----------------------|--------------|-----------:|----------:|-------:|')
        for dirpath, cadence_label in noisy_dirs:
            for demod, filt, label in COMBOS:
                path = f'{dirpath}/{mode_name}_{demod}_{filt}.bmp'
                if not os.path.exists(path):
                    continue
                m = measure(path, info, ref_color)
                psnr_str = 'inf' if m['psnr'] == float('inf') else f"{m['psnr']:.2f}"
                ssim_str = f"{m['ssim']:.3f}"
                print(f"| {label:<20} | {cadence_label:<12} | {m['decoded_total_rows']:>10} | {psnr_str:>9} | {ssim_str:>6} |")


if __name__ == '__main__':
    main()
