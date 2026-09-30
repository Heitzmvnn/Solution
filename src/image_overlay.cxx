#include "image_overlay.hxx"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <utility>

#include "exception_messages.hxx"

using namespace ExceptionMessages;

ImageOverlay::ImageOverlay(YuvFrame overlay)
    : overlay_(std::move(overlay)) {
    if (!overlay_.is_valid())
        throw std::invalid_argument(kInvalidOverlayFrame);

    if ((overlay_.width() & 1) != 0 || (overlay_.height() & 1) != 0)
        throw std::invalid_argument(kOverlayDimensionsMustBeEven);
}

void ImageOverlay::ApplyTo(YuvFrame& target) const {
    if (kAlpha == 0)
        return;

    if (!target.is_valid())
        throw std::invalid_argument(kInvalidTargetFrame);

    BlendPlane(target.data_y(), target.stride_y(),
        target.width(), target.height(),
        overlay_.data_y(), overlay_.stride_y(),
        overlay_.width(), overlay_.height(),
        x_, y_, kAlpha);

    const std::int32_t cx = x_ / 2;
    const std::int32_t cy = y_ / 2;

    BlendPlane(target.data_u(), target.stride_u(),
        target.chroma_width(), target.chroma_height(),
        overlay_.data_u(), overlay_.stride_u(),
        overlay_.chroma_width(), overlay_.chroma_height(),
        cx, cy, kAlpha);

    BlendPlane(target.data_v(), target.stride_v(),
        target.chroma_width(), target.chroma_height(),
        overlay_.data_v(), overlay_.stride_v(),
        overlay_.chroma_width(), overlay_.chroma_height(),
        cx, cy, kAlpha);
}

void ImageOverlay::ApplyToAll(YuvFile& file, std::int32_t x, std::int32_t y) {
    if ((x & 1) != 0 || (y & 1) != 0)
        throw std::invalid_argument(kPositionMustBeEven);

    x_ = x;
    y_ = y;

    for (std::size_t i = 0; i < file.frame_count(); ++i)
        ApplyTo(file.frame(i));
}

YuvFrame ImageOverlay::Blended(const YuvFrame& target) const {
    YuvFrame copy = target;
    ApplyTo(copy);

    return copy;
}

void ImageOverlay::BlendPlane(std::uint8_t* dst, std::int32_t dst_stride,
    std::int32_t dst_w, std::int32_t dst_h,
    const std::uint8_t* src,
    std::int32_t src_stride,
    std::int32_t src_w, std::int32_t src_h,
    std::int32_t offset_x, std::int32_t offset_y,
    std::uint8_t alpha) {
    const std::int32_t sx0 = std::max(0, -offset_x);
    const std::int32_t sy0 = std::max(0, -offset_y);
    const std::int32_t sx1 = std::min(src_w, dst_w - offset_x);
    const std::int32_t sy1 = std::min(src_h, dst_h - offset_y);

    if (sx0 >= sx1 || sy0 >= sy1)
        return;

    const bool opaque = (alpha == 255);

    for (std::int32_t sy = sy0; sy < sy1; ++sy) {
        std::uint8_t* d_row = dst + static_cast<std::size_t>(offset_y + sy) * dst_stride;
        const std::uint8_t* s_row = src + static_cast<std::size_t>(sy) * src_stride;

        if (opaque) {
            std::memcpy(d_row + offset_x + sx0, s_row + sx0, static_cast<std::size_t>(sx1 - sx0));
        }
        else {
            for (std::int32_t sx = sx0; sx < sx1; ++sx) {
                const std::int32_t d = offset_x + sx;
                d_row[d] = Blend(d_row[d], s_row[sx], alpha);
            }
        }
    }
}

std::uint8_t ImageOverlay::Blend(std::uint8_t dst, std::uint8_t src, std::uint8_t alpha) noexcept {
    const std::int32_t ia = 255 - static_cast<std::int32_t>(alpha);
    const std::int32_t sum = static_cast<std::int32_t>(dst) * ia + static_cast<std::int32_t>(src) * alpha;

    return static_cast<std::uint8_t>((sum + 127) / 255);
}