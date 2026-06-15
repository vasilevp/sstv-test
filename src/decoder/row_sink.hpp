#pragma once
#include <cstdint>
#include <span>

// Push interface for decoded scanlines. Every time the Decoder finishes a
// row's worth of pixels it calls `row()` once; when the stream ends it calls
// `finish()` exactly once so implementations that buffer (e.g. BMP files
// that need a final header patch) can flush.
//
// The decoder no longer accumulates the whole image in memory — embedding
// targets can route rows straight to a TFT framebuffer, an SD card, or a
// network socket without ever holding more than one row at a time.
class RowSink
{
public:
	virtual ~RowSink() = default;

	// One scanline of width*3 bytes in RGB order (R, G, B, R, G, B, ...).
	// Rows arrive top-down: row 0 first, row N-1 last. Implementations may
	// reorder for their own format (e.g. BMP's bottom-up convention) by
	// buffering or by setting up a top-down DIB header up front.
	virtual void row(std::span<const std::uint8_t> rgb) = 0;

	// No more rows will arrive. Implementations should flush any pending
	// state (close files, patch headers, etc.).
	virtual void finish() = 0;
};
