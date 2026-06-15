#pragma once
#include <cstdint>
#include <span>

// Random-access row interface for the encoder's pixel input. Mode encoders
// walk the image top-down (Robot/Martin/Scottie one row at a time; PD pairs
// adjacent rows for chroma averaging; Robot 36/72 likewise need row i and
// row i±1 for chroma averaging), so a small two-row cache satisfies every
// access pattern without holding the whole image in memory.
//
// width() and height() are constant for the source's lifetime; row(y)
// returns a borrowed view of `width()*3` bytes in RGB order (R, G, B,
// R, G, B, ...). The returned span is invalidated by the next row() call.
class RowSource
{
public:
	virtual ~RowSource() = default;

	virtual std::uint32_t width() const = 0;
	virtual std::uint32_t height() const = 0;

	// Read row `y` (0 = top of image, height()-1 = bottom). The returned
	// span points at memory owned by the source — copy out if you need it
	// after the next row() call.
	virtual std::span<const std::uint8_t> row(std::uint32_t y) = 0;
};
