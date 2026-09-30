#ifndef BITMAP_TO_YUV_FRAME_CONVERTER_H_
#define BITMAP_TO_YUV_FRAME_CONVERTER_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "bitmap_file.hxx"
#include "yuv_frame.hxx"

class BitmapToYuvFrameConverter {
public:
    BitmapToYuvFrameConverter(const BitmapFile& bitmap_file);
    
    YuvFrame& frame() noexcept { return frame_; }
    const YuvFrame& frame() const noexcept { return frame_; }

    YuvFrame& Convert();
    YuvFrame& ConvertParallel(std::size_t num_threads = 0);
    YuvFrame& ConvertSimd();
    void Prepare();
    void RgbToYuvRows(std::int32_t y_begin, std::int32_t y_end);
    void DownsampleChromaRows(std::int32_t cy_begin, std::int32_t cy_end);

private:
    static std::uint8_t ClampToU8(std::int32_t v) noexcept;
    void RgbToYuvRowsScalar(std::int32_t y_begin, std::int32_t y_end);
    void RgbToYuvRowsSimd(std::int32_t y_begin, std::int32_t y_end);

    static void RgbToYuv(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t& y, std::uint8_t& u, std::uint8_t& v);

#if YUV_HAVE_X86_SIMD
    YUV_TARGET_SSSE3
        void RgbToYuvRowsSsse3(std::int32_t y_begin, std::int32_t y_end);

#if defined(__AVX2__)
    YUV_TARGET_AVX2
        void RgbToYuvRowsAvx2(std::int32_t y_begin, std::int32_t y_end);
#endif  // __AVX2__

    static bool CpuHasAvx2() noexcept;
#endif  // YUV_HAVE_X86_SIMD

    std::int32_t width_ = 0;
    std::int32_t height_ = 0;

    std::vector<std::uint8_t> u444_;
    std::vector<std::uint8_t> v444_;

    BitmapFile bitmap_file_;
    YuvFrame frame_;
};

#endif // BITMAP_TO_YUV_FRAME_CONVERTER_H_