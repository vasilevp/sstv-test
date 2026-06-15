#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>
#include <vector>

#include "row_source.hpp"

// Streaming 24-bit BMP reader. Parses the BMP/DIB headers once at open,
// then seeks to each requested row on demand. Holds the two most-recently
// read rows in an LRU cache — enough for every mode encoder's access
// pattern (Martin/Scottie revisit the same row up to three times; Robot
// and PD pair adjacent rows for chroma averaging). The whole-image
// allocation that loadbmp_decode_file was doing is gone; steady-state
// memory is `2 * width * 3` bytes (under 2 KB for 320-wide images).
//
// Only 24-bit uncompressed BMPs are supported — same as what the project's
// encoder has always assumed. Both row orientations are handled: standard
// bottom-up files seek to height-1-y * rowBytes; top-down files (negative
// DIB height) seek to y * rowBytes.
class BMPRowSource : public RowSource
{
public:
	explicit BMPRowSource(const std::string &path);
	~BMPRowSource() override;

	BMPRowSource(const BMPRowSource &) = delete;
	BMPRowSource &operator=(const BMPRowSource &) = delete;

	std::uint32_t width() const override { return imgWidth; }
	std::uint32_t height() const override { return imgHeight; }

	std::span<const std::uint8_t> row(std::uint32_t y) override;

private:
	void loadRow(std::uint32_t y, std::vector<std::uint8_t> &dst);

	std::FILE *file = nullptr;
	std::uint32_t imgWidth = 0;
	std::uint32_t imgHeight = 0;
	std::uint32_t pixelOffset = 0; // file offset of the first pixel byte
	std::uint32_t rowBytes = 0;    // on-disk row stride (padded to 4 bytes)
	bool topDown = false;          // true if DIB height was negative

	// Two-row LRU. cachedRow[slot] holds the row index currently in
	// cachedData[slot], or UINT32_MAX when the slot is empty. mruSlot is
	// the most-recently used slot; the other is the eviction candidate.
	std::array<std::vector<std::uint8_t>, 2> cachedData;
	std::array<std::uint32_t, 2> cachedRow{UINT32_MAX, UINT32_MAX};
	std::uint8_t mruSlot = 0;
};
