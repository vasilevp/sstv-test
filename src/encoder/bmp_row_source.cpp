#include "bmp_row_source.hpp"

#include <cstring>
#include <stdexcept>

namespace
{
	std::uint16_t readU16LE(const unsigned char *p)
	{
		return std::uint16_t(p[0]) | (std::uint16_t(p[1]) << 8);
	}

	std::uint32_t readU32LE(const unsigned char *p)
	{
		return std::uint32_t(p[0]) |
		       (std::uint32_t(p[1]) << 8) |
		       (std::uint32_t(p[2]) << 16) |
		       (std::uint32_t(p[3]) << 24);
	}

	std::int32_t readI32LE(const unsigned char *p)
	{
		return static_cast<std::int32_t>(readU32LE(p));
	}
}

BMPRowSource::BMPRowSource(const std::string &path)
{
	file = std::fopen(path.c_str(), "rb");
	if (!file)
		throw std::runtime_error("Failed to open BMP for reading: " + path);

	// Parse the 14-byte BMP file header + 40-byte BITMAPINFOHEADER. The
	// project only writes (and only ever read, via loadbmp) plain 24-bit
	// uncompressed BMPs, so we reject anything else explicitly.
	unsigned char hdr[54];
	if (std::fread(hdr, 1, sizeof(hdr), file) != sizeof(hdr))
		throw std::runtime_error("BMP header read failed: " + path);

	if (hdr[0] != 'B' || hdr[1] != 'M')
		throw std::runtime_error("Not a BMP (missing BM signature): " + path);

	pixelOffset = readU32LE(hdr + 10);
	const std::uint32_t dibSize = readU32LE(hdr + 14);
	if (dibSize < 40)
		throw std::runtime_error("Unsupported BMP: DIB header too small in " + path);

	imgWidth = readU32LE(hdr + 18);
	const std::int32_t signedHeight = readI32LE(hdr + 22);
	topDown = signedHeight < 0;
	imgHeight = topDown ? std::uint32_t(-signedHeight) : std::uint32_t(signedHeight);

	const std::uint16_t planes = readU16LE(hdr + 26);
	const std::uint16_t bpp = readU16LE(hdr + 28);
	const std::uint32_t compression = readU32LE(hdr + 30);
	if (planes != 1 || bpp != 24 || compression != 0)
		throw std::runtime_error(
			"Unsupported BMP variant (need 24-bit uncompressed): " + path);

	rowBytes = ((imgWidth * 3 + 3) / 4) * 4;

	// Pre-size the LRU buffers so row() never reallocates.
	cachedData[0].resize(std::size_t(imgWidth) * 3);
	cachedData[1].resize(std::size_t(imgWidth) * 3);
}

BMPRowSource::~BMPRowSource()
{
	if (file)
		std::fclose(file);
}

std::span<const std::uint8_t> BMPRowSource::row(std::uint32_t y)
{
	if (y >= imgHeight)
		throw std::runtime_error("BMPRowSource::row out of range");

	for (std::uint8_t slot = 0; slot < 2; ++slot)
	{
		if (cachedRow[slot] == y)
		{
			mruSlot = slot;
			return std::span<const std::uint8_t>(cachedData[slot]);
		}
	}

	// Cache miss: load into the LRU slot (the one that wasn't most-
	// recently used) so the row we just touched survives the next call.
	const std::uint8_t evict = 1 - mruSlot;
	loadRow(y, cachedData[evict]);
	cachedRow[evict] = y;
	mruSlot = evict;
	return std::span<const std::uint8_t>(cachedData[evict]);
}

void BMPRowSource::loadRow(std::uint32_t y, std::vector<std::uint8_t> &dst)
{
	const std::uint32_t fileRow = topDown ? y : (imgHeight - 1 - y);
	const long offset = long(pixelOffset) + long(fileRow) * long(rowBytes);
	if (std::fseek(file, offset, SEEK_SET) != 0)
		throw std::runtime_error("BMPRowSource::loadRow seek failed");

	// Read width*3 BGR bytes (skipping the trailing padding implicitly:
	// we only ask for the pixel bytes themselves; the seek above already
	// landed us at the start of the row).
	const std::size_t want = std::size_t(imgWidth) * 3;
	if (std::fread(dst.data(), 1, want, file) != want)
		throw std::runtime_error("BMPRowSource::loadRow short read");

	// On-disk byte order is BGR; the rest of the encoder expects RGB
	// triplets (matching what loadbmp_decode_file used to hand back).
	for (std::size_t x = 0; x < imgWidth; ++x)
	{
		const std::uint8_t b = dst[3 * x + 0];
		const std::uint8_t r = dst[3 * x + 2];
		dst[3 * x + 0] = r;
		dst[3 * x + 2] = b;
	}
}
