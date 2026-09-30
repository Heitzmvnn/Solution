#ifndef BITMAP_FILE_H_
#define BITMAP_FILE_H_

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#pragma pack(push, 1)
struct BitmapHeader
{
	std::uint16_t signature = 0;
	std::uint32_t file_size = 0;
	std::uint16_t reserved_1 = 0;
	std::uint16_t reserved_2 = 0;
	std::uint32_t data_offset = 0;
};

struct BitmapInfoHeader
{
	std::uint32_t	header_size = 0;
	std::int32_t	width = 0;
	std::int32_t	height = 0;
	std::uint16_t	planes = 0;
	std::uint16_t	bit_count = 0;
	std::uint32_t	compression = 0;
	std::uint32_t	image_size = 0;
	std::int32_t	x_pixels_per_m = 0;
	std::int32_t	y_pixels_per_m = 0;
	std::uint32_t	colors_used = 0;
	std::uint32_t	colors_important = 0;
};
#pragma pack(pop)

struct PixelBgr
{
	std::uint8_t b = 0;
	std::uint8_t g = 0;
	std::uint8_t r = 0;
};

class BitmapFile
{
public:
	BitmapFile(const std::string& file_path);

	std::int32_t width()                 const noexcept { return info_header_.width; }
	std::int32_t height()                const noexcept { return info_header_.height; }
	std::uint16_t bit_count()            const noexcept { return info_header_.bit_count; }
	const std::vector<PixelBgr>& data()  const noexcept { return data_; }

private:
	static constexpr std::uint16_t kBitmapFileSignature = 0x4D42;
	static constexpr std::uint32_t kSupportedInfoHeaderSize = 40;
	static constexpr std::uint16_t kSupportedBitCount = 24;
	static constexpr std::uint32_t kSupportedCompressionMode = 0;
	static constexpr std::uint16_t kSupportedPlanesCount = 1;

	static constexpr std::size_t kBytesPerPixel = 3;
	static constexpr std::size_t kRowAlignment = 4;
	
	void ReadHeaders(std::ifstream& file);
	void ReadPixelData(std::ifstream& file);

	BitmapHeader header_;
	BitmapInfoHeader info_header_;
	std::vector<PixelBgr> data_;
};

#endif // BITMAP_FILE_H_