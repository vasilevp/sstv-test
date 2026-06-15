#pragma once
#include <cstdint>
#include <cstdio>
#include <span>
#include <string>

#include "row_sink.hpp"

// Streaming row sink that writes a 24-bit BMP top-down. Avoids buffering the
// whole image: the BMP header is written at construction with a placeholder
// height (-1 in the negative-height DIB convention, which marks the row
// order as top-down so consumers don't have to reverse-Y), every incoming
// row is byte-swapped RGB → BGR and pushed straight to disk, and finish()
// seeks back to patch the real height and file-size fields in the header.
//
// The output file must therefore be seekable — pipes/sockets can't use this
// sink. The trade-off is that we hold zero rows in memory; on a tight target
// you'd subclass RowSink to push rows somewhere that doesn't need a final
// patch (a TFT, a fresh-each-frame file).
class BMPRowSink : public RowSink
{
public:
	// Open `path` for writing and emit a header sized for `width` columns
	// of 24-bpp pixels with as-yet-unknown row count.
	BMPRowSink(const std::string &path, std::uint32_t width);
	~BMPRowSink() override;

	BMPRowSink(const BMPRowSink &) = delete;
	BMPRowSink &operator=(const BMPRowSink &) = delete;

	void row(std::span<const std::uint8_t> rgb) override;
	void finish() override;

	std::uint32_t rowCount() const { return rows; }

private:
	std::string path;
	std::FILE *file = nullptr;
	std::uint32_t width;
	std::uint32_t rows = 0;
	std::uint32_t padding;  // bytes appended after each row to reach 4-byte alignment
	bool finished = false;
};
