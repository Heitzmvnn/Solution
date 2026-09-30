#include "bitmap_file.hxx"

#include <cstddef>
#include <stdexcept>

#include "exception_messages.hxx"

using namespace ExceptionMessages;

BitmapFile::BitmapFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file)
        throw std::runtime_error(kFailedToOpenFile + file_path);

    ReadHeaders(file);
    ReadPixelData(file);
}

void BitmapFile::ReadHeaders(std::ifstream& file) {
    file.read(reinterpret_cast<char*>(&header_), static_cast<std::streamsize>(sizeof(BitmapHeader)));
    if (!file)
        throw std::runtime_error(kFailedToReadBmpHeader);

    if (header_.signature != kBitmapFileSignature)
        throw std::runtime_error(kFileIsNotBmpMissingBmSignature);

    file.read(reinterpret_cast<char*>(&info_header_), static_cast<std::streamsize>(sizeof(BitmapInfoHeader)));
    if (!file)
        throw std::runtime_error(kFailedToReadBmpInfoHeader);

    if (info_header_.header_size != kSupportedInfoHeaderSize)
        throw std::runtime_error(kOnlyBmpInfoHeader40BytesSupported);

    if (info_header_.bit_count != kSupportedBitCount)
        throw std::runtime_error(kOnly24BitBmpSupported);

    if (info_header_.planes != kSupportedPlanesCount)
        throw std::runtime_error(kOnlySinglePlaneBmpSupported);

    if (info_header_.compression != kSupportedCompressionMode)
        throw std::runtime_error(kOnlyUncompressedBmpBiRgbSupported);

    const std::uint64_t headers_end = static_cast<std::uint64_t>(sizeof(BitmapHeader)) + info_header_.header_size;
    if (static_cast<std::uint64_t>(header_.data_offset) < headers_end)
        throw std::runtime_error(kDataOffsetOverlapsHeaders);
    
    if (header_.file_size != 0 && static_cast<std::uint64_t>(header_.file_size) < static_cast<std::uint64_t>(header_.data_offset))
        throw std::runtime_error(kFileSizeSmallerThanDataOffset);
}

void BitmapFile::ReadPixelData(std::ifstream& file) {
    const std::int32_t width = info_header_.width;
    const std::int32_t height = info_header_.height;
    const std::size_t width_sz = static_cast<std::size_t>(width);

    if (width <= 0 || height == 0)
        throw std::runtime_error(kInvalidBmpDimensions);

    file.seekg(static_cast<std::streamoff>(header_.data_offset), std::ios::beg);
    if (!file)
        throw std::runtime_error(kInvalidDataOffsetInBmp);

    const std::int32_t abs_height = height < 0 ? -height : height;
    
    const std::size_t row_stride = ((width_sz * kBytesPerPixel + kRowAlignment - 1) / kRowAlignment) * kRowAlignment;
    std::vector<std::uint8_t> row(row_stride);
    
    const std::uint64_t required_bytes = static_cast<std::uint64_t>(row_stride) * static_cast<std::uint64_t>(abs_height);

    const std::streampos saved_pos = file.tellg();
    file.seekg(0, std::ios::end);
    const std::streampos end_pos = file.tellg();
    file.seekg(saved_pos);

    if (end_pos < 0)
        throw std::runtime_error(kFailedToReadBmpPixelRow);

    const std::uint64_t actual_file_size = static_cast<std::uint64_t>(end_pos);
    const std::uint64_t data_start = static_cast<std::uint64_t>(header_.data_offset);

    if (actual_file_size < data_start || actual_file_size - data_start < required_bytes)
        throw std::runtime_error(kTruncatedBmpPixelData);
    
    data_.resize(width_sz * static_cast<std::size_t>(abs_height));

    for (std::int32_t y = 0; y < abs_height; ++y) {
        file.read(reinterpret_cast<char*>(row.data()), static_cast<std::streamsize>(row_stride));
        if (!file)
            throw std::runtime_error(kFailedToReadBmpPixelRow);

        const std::int32_t target_y = (height > 0) ? (height - 1 - y) : y;

        for (std::int32_t x = 0; x < width; ++x) {
            const std::size_t x_sz = static_cast<std::size_t>(x);
            const std::size_t src = x_sz * kBytesPerPixel;

            PixelBgr pixel;
            pixel.b = row[src];
            pixel.g = row[src + 1];
            pixel.r = row[src + 2];

            const std::size_t destination = static_cast<std::size_t>(target_y) * width_sz + x_sz;
            data_[destination] = pixel;
        }
    }
}