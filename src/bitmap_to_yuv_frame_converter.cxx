#include "bitmap_to_yuv_frame_converter.hxx"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <vector>

#include "exception_messages.hxx"

using namespace ExceptionMessages;

BitmapToYuvFrameConverter::BitmapToYuvFrameConverter(const BitmapFile& bitmap_file)
    : bitmap_file_(bitmap_file) {
}

YuvFrame& BitmapToYuvFrameConverter::Convert() {
    const std::int32_t w = bitmap_file_.width();
    const std::int32_t raw_h = bitmap_file_.height();
    const std::int32_t h = std::abs(raw_h);

    if (w <= 0 || h <= 0)
        throw std::runtime_error(kInvalidBmpSize);

    if (w % 2 != 0 || h % 2 != 0)
        throw std::runtime_error(kWidthAndHeightMustBeEvenForI420);

    if (bitmap_file_.bit_count() != 24)
        throw std::runtime_error(kOnly24BitBmpSupported);

    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
    if (bgr.size() < expected)
        throw std::runtime_error(kBmpPixelBufferTooSmall);

    frame_.Allocate(w, h);

    const std::int32_t stride_y = frame_.stride_y();
    const std::int32_t stride_u = frame_.stride_u();
    const std::int32_t stride_v = frame_.stride_v();

    std::vector<std::uint8_t> u444(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
    std::vector<std::uint8_t> v444(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));

    for (std::int32_t y = 0; y < h; ++y) {
        const PixelBgr* src_row = bgr.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w);

        std::uint8_t* y_dst = frame_.data_y() + static_cast<std::size_t>(y) * stride_y;
        std::uint8_t* u_dst = u444.data() + static_cast<std::size_t>(y) * w;
        std::uint8_t* v_dst = v444.data() + static_cast<std::size_t>(y) * w;

        for (std::int32_t x = 0; x < w; ++x) {
            const std::uint8_t b = src_row[x].b;
            const std::uint8_t g = src_row[x].g;
            const std::uint8_t r = src_row[x].r;

            std::uint8_t yy = 0, uu = 0, vv = 0;
            RgbToYuv(r, g, b, yy, uu, vv);

            y_dst[x] = yy;
            u_dst[x] = uu;
            v_dst[x] = vv;
        }
    }

    const std::int32_t cw = w / 2;
    const std::int32_t ch = h / 2;

    for (std::int32_t cy = 0; cy < ch; ++cy) {
        std::uint8_t* u_dst = frame_.data_u() + static_cast<std::size_t>(cy) * stride_u;
        std::uint8_t* v_dst = frame_.data_v() + static_cast<std::size_t>(cy) * stride_v;

        for (std::int32_t cx = 0; cx < cw; ++cx) {
            const std::int32_t x0 = cx * 2;
            const std::int32_t y0 = cy * 2;

            const std::uint8_t* u_row0 = u444.data() + static_cast<std::size_t>(y0) * w;
            const std::uint8_t* u_row1 = u444.data() + static_cast<std::size_t>(y0 + 1) * w;
            const std::uint8_t* v_row0 = v444.data() + static_cast<std::size_t>(y0) * w;
            const std::uint8_t* v_row1 = v444.data() + static_cast<std::size_t>(y0 + 1) * w;

            const std::int32_t u_sum = static_cast<std::int32_t>(u_row0[x0]) + static_cast<std::int32_t>(u_row0[x0 + 1]) +
                static_cast<std::int32_t>(u_row1[x0]) + static_cast<std::int32_t>(u_row1[x0 + 1]);
            const std::int32_t v_sum = static_cast<std::int32_t>(v_row0[x0]) + static_cast<std::int32_t>(v_row0[x0 + 1]) +
                static_cast<std::int32_t>(v_row1[x0]) + static_cast<std::int32_t>(v_row1[x0 + 1]);

            u_dst[cx] = static_cast<std::uint8_t>((u_sum + 2) >> 2);
            v_dst[cx] = static_cast<std::uint8_t>((v_sum + 2) >> 2);
        }
    }

    return frame_;
}

YuvFrame& BitmapToYuvFrameConverter::ConvertParallel(std::size_t num_threads) {
    Prepare();

    const std::int32_t h = height_;

    if (num_threads == 0)
        num_threads = std::thread::hardware_concurrency();

    if (num_threads == 0)
        num_threads = 1;

    num_threads = std::min<std::size_t>(num_threads, static_cast<std::size_t>(h / 2));
    if (num_threads < 1)
        num_threads = 1;

    const std::int32_t rows_per_thread = ((h / 2) + static_cast<std::int32_t>(num_threads) - 1) / static_cast<std::int32_t>(num_threads);
    const std::int32_t row_step = rows_per_thread * 2;

    std::vector<std::thread> workers;
    workers.reserve(num_threads);

    for (std::size_t t = 0; t < num_threads; ++t) {
        const std::int32_t y_begin = static_cast<std::int32_t>(t) * row_step;
        const std::int32_t y_end = std::min(h, y_begin + row_step);
        if (y_begin >= y_end)
            break;

        workers.emplace_back([this, y_begin, y_end]() {
            RgbToYuvRows(y_begin, y_end);
            DownsampleChromaRows(y_begin / 2, y_end / 2);
            });
    }

    for (std::thread& th : workers)
        th.join();

    return frame_;
}

void BitmapToYuvFrameConverter::Prepare() {
    const std::int32_t w = bitmap_file_.width();
    const std::int32_t raw_h = bitmap_file_.height();
    const std::int32_t h = std::abs(raw_h);

    if (w <= 0 || h <= 0)
        throw std::runtime_error(kInvalidBmpSize);

    if (w % 2 != 0 || h % 2 != 0)
        throw std::runtime_error(kWidthAndHeightMustBeEvenForI420);

    if (bitmap_file_.bit_count() != 24)
        throw std::runtime_error(kOnly24BitBmpSupported);

    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::size_t expected = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);

    if (bgr.size() < expected)
        throw std::runtime_error(kBmpPixelBufferTooSmall);

    width_ = w;
    height_ = h;

    frame_.Allocate(w, h);

    u444_.assign(static_cast<std::size_t>(w) * h, 128);
    v444_.assign(static_cast<std::size_t>(w) * h, 128);
}

void BitmapToYuvFrameConverter::RgbToYuvRows(std::int32_t y_begin, std::int32_t y_end) {
    const std::int32_t w = width_;
    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::int32_t stride_y = frame_.stride_y();

    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const PixelBgr* src_row = bgr.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w);

        std::uint8_t* y_dst = frame_.data_y() + static_cast<std::size_t>(y) * stride_y;
        std::uint8_t* u_dst = u444_.data() + static_cast<std::size_t>(y) * w;
        std::uint8_t* v_dst = v444_.data() + static_cast<std::size_t>(y) * w;

        for (std::int32_t x = 0; x < w; ++x) {
            std::uint8_t yy = 0, uu = 0, vv = 0;
            RgbToYuv(src_row[x].r, src_row[x].g, src_row[x].b, yy, uu, vv);

            y_dst[x] = yy;
            u_dst[x] = uu;
            v_dst[x] = vv;
        }
    }
}

void BitmapToYuvFrameConverter::DownsampleChromaRows(std::int32_t cy_begin, std::int32_t cy_end) {
    const std::int32_t w = width_;
    const std::int32_t cw = w / 2;

    const std::int32_t stride_u = frame_.stride_u();
    const std::int32_t stride_v = frame_.stride_v();

    for (std::int32_t cy = cy_begin; cy < cy_end; ++cy) {
        std::uint8_t* u_dst = frame_.data_u() + static_cast<std::size_t>(cy) * stride_u;
        std::uint8_t* v_dst = frame_.data_v() + static_cast<std::size_t>(cy) * stride_v;

        const std::int32_t y0 = cy * 2;

        const std::uint8_t* u_row0 = u444_.data() + static_cast<std::size_t>(y0) * w;
        const std::uint8_t* u_row1 = u444_.data() + static_cast<std::size_t>(y0 + 1) * w;
        const std::uint8_t* v_row0 = v444_.data() + static_cast<std::size_t>(y0) * w;
        const std::uint8_t* v_row1 = v444_.data() + static_cast<std::size_t>(y0 + 1) * w;

        for (std::int32_t cx = 0; cx < cw; ++cx) {
            const std::int32_t x0 = cx * 2;

            const std::int32_t u_sum = static_cast<std::int32_t>(u_row0[x0]) + static_cast<std::int32_t>(u_row0[x0 + 1]) +
                static_cast<std::int32_t>(u_row1[x0]) + static_cast<std::int32_t>(u_row1[x0 + 1]);
            const std::int32_t v_sum = static_cast<std::int32_t>(v_row0[x0]) + static_cast<std::int32_t>(v_row0[x0 + 1]) +
                static_cast<std::int32_t>(v_row1[x0]) + static_cast<std::int32_t>(v_row1[x0 + 1]);

            u_dst[cx] = static_cast<std::uint8_t>((u_sum + 2) >> 2);
            v_dst[cx] = static_cast<std::uint8_t>((v_sum + 2) >> 2);
        }
    }
}

YuvFrame& BitmapToYuvFrameConverter::ConvertSimd() {
    Prepare();

    const std::int32_t h = height_;
    const std::int32_t ch = h / 2;

    RgbToYuvRowsSimd(0, h);
    DownsampleChromaRows(0, ch);

    return frame_;
}

void BitmapToYuvFrameConverter::RgbToYuv(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t& y, std::uint8_t& u, std::uint8_t& v) {
    const std::int32_t ri = r;
    const std::int32_t gi = g;
    const std::int32_t bi = b;

    const std::int32_t yy = (77 * ri + 150 * gi + 29 * bi) >> 8;
    const std::int32_t uu = ((-43 * ri - 85 * gi + 128 * bi) >> 8) + 128;
    const std::int32_t vv = ((128 * ri - 107 * gi - 21 * bi) >> 8) + 128;

    y = ClampToU8(yy);
    u = ClampToU8(uu);
    v = ClampToU8(vv);
}

std::uint8_t BitmapToYuvFrameConverter::ClampToU8(std::int32_t v) noexcept {
    if (v < 0)
        return 0;

    if (v > 255)
        return 255;

    return static_cast<std::uint8_t>(v);
}