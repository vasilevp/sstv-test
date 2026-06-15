#include "bmp_row_sink.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace
{
	void writeU32LE(unsigned char *p, std::uint32_t v)
	{
		p[0] = std::uint8_t(v);
		p[1] = std::uint8_t(v >> 8);
		p[2] = std::uint8_t(v >> 16);
		p[3] = std::uint8_t(v >> 24);
	}

	void writeI32LE(unsigned char *p, std::int32_t v)
	{
		writeU32LE(p, static_cast<std::uint32_t>(v));
	}
}

BMPRowSink::BMPRowSink(const std::string &path, std::uint32_t width)
	: path(path),
	  width(width),
	  padding((4 - (width * 3) % 4) % 4)
{
	file = std::fopen(path.c_str(), "wb");
	if (!file)
		throw std::runtime_error("Failed to open BMP for writing: " + path);

	// 14-byte BMP file header + 40-byte BITMAPINFOHEADER. The file size and
	// the height field are placeholders — finish() patches them once we know
	// how many rows we actually got.
	unsigned char hdr[54] = {};
	hdr[0] = 'B';
	hdr[1] = 'M';
	// file size at offset 2 — placeholder (54 only) until finish() patches.
	writeU32LE(hdr + 2, 54);
	// pixel-data offset at 10 = 54.
	writeU32LE(hdr + 10, 54);
	// DIB header size at 14 = 40.
	writeU32LE(hdr + 14, 40);
	writeU32LE(hdr + 18, width);
	// height at 22, written negative to mark rows as top-down; placeholder
	// -1 until finish() writes the real (negative) row count.
	writeI32LE(hdr + 22, -1);
	// planes at 26 = 1, bpp at 28 = 24.
	hdr[26] = 1;
	hdr[28] = 24;
	// remaining DIB fields (compression, image size, ppm, palette) stay 0.

	if (std::fwrite(hdr, 1, sizeof(hdr), file) != sizeof(hdr))
	{
		std::fclose(file);
		file = nullptr;
		throw std::runtime_error("Failed to write BMP header: " + path);
	}
}

BMPRowSink::~BMPRowSink()
{
	// Best-effort close — if the caller forgot to call finish() we still
	// release the file, though the header will be wrong.
	if (file)
		std::fclose(file);
}

void BMPRowSink::row(std::span<const std::uint8_t> rgb)
{
	if (!file || finished)
		throw std::runtime_error("BMPRowSink::row() after finish or close");
	if (rgb.size() != std::size_t(width) * 3)
		throw std::runtime_error("BMPRowSink::row() given wrong-width row");

	// BMP stores pixels as BGR. Swap into a small stack buffer one row at a
	// time so we never hold more than ~960 bytes in flight (320 px × 3).
	std::vector<std::uint8_t> bgr(rgb.size());
	for (std::size_t x = 0; x < width; ++x)
	{
		bgr[3 * x + 0] = rgb[3 * x + 2];
		bgr[3 * x + 1] = rgb[3 * x + 1];
		bgr[3 * x + 2] = rgb[3 * x + 0];
	}
	if (std::fwrite(bgr.data(), 1, bgr.size(), file) != bgr.size())
		throw std::runtime_error("BMPRowSink::row() short write: " + path);

	if (padding > 0)
	{
		constexpr unsigned char zero[3] = {0, 0, 0};
		if (std::fwrite(zero, 1, padding, file) != padding)
			throw std::runtime_error("BMPRowSink::row() short pad write: " + path);
	}
	++rows;
}

void BMPRowSink::finish()
{
	if (!file || finished)
		return;
	finished = true;

	const std::uint32_t rowBytes = width * 3 + padding;
	const std::uint32_t size = 54 + rowBytes * rows;
	unsigned char fileSizeLE[4];
	writeU32LE(fileSizeLE, size);
	std::fseek(file, 2, SEEK_SET);
	std::fwrite(fileSizeLE, 1, 4, file);

	// Negative height marks rows as top-down so consumers know row 0 is the
	// top of the image, not the bottom.
	unsigned char heightLE[4];
	writeI32LE(heightLE, -static_cast<std::int32_t>(rows));
	std::fseek(file, 22, SEEK_SET);
	std::fwrite(heightLE, 1, 4, file);

	std::fclose(file);
	file = nullptr;
}
