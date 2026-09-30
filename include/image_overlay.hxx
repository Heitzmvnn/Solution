#ifndef IMAGE_OVERLAY_H_
#define IMAGE_OVERLAY_H_

#include <cstdint>

#include "yuv_file.hxx"
#include "yuv_frame.hxx"

class ImageOverlay {
public:
    explicit ImageOverlay(YuvFrame overlay);

    YuvFrame& image() noexcept { return overlay_; }
    const YuvFrame& image() const noexcept { return overlay_; }

    std::int32_t width() const noexcept { return overlay_.width(); }
    std::int32_t height() const noexcept { return overlay_.height(); }

    std::int32_t x() const noexcept { return x_; }
    std::int32_t y() const noexcept { return y_; }

    void ApplyTo(YuvFrame& target) const;
    void ApplyToAll(YuvFile& file, std::int32_t x, std::int32_t y);
    YuvFrame Blended(const YuvFrame& target) const;

private:
    static constexpr std::uint8_t kAlpha = 255;

    static void BlendPlane(std::uint8_t* dst, std::int32_t dst_stride,
        std::int32_t dst_w, std::int32_t dst_h,
        const std::uint8_t* src, std::int32_t src_stride,
        std::int32_t src_w, std::int32_t src_h,
        std::int32_t offset_x, std::int32_t offset_y,
        std::uint8_t alpha);
    
    static std::uint8_t Blend(std::uint8_t dst, std::uint8_t src,
        std::uint8_t alpha) noexcept;

    YuvFrame overlay_;
    std::int32_t x_ = 0;
    std::int32_t y_ = 0;
};

#endif // IMAGE_OVERLAY_H_