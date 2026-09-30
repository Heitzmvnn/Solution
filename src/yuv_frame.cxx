#include "yuv_frame.hxx"

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "exception_messages.hxx"

using namespace ExceptionMessages;

YuvFrame::YuvFrame(std::int32_t width, std::int32_t height) {
    Allocate(width, height);
}

void YuvFrame::Allocate(std::int32_t width, std::int32_t height) {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument(kInvalidDimensions);

    if (width_ == width && height_ == height && !plane_y_.empty())
        return;

    width_ = width;
    height_ = height;

    const std::int32_t cw = chroma_width();
    const std::int32_t ch = chroma_height();

    plane_y_.assign(static_cast<std::size_t>(width) * height, 0);
    plane_u_.assign(static_cast<std::size_t>(cw) * ch, 128);
    plane_v_.assign(static_cast<std::size_t>(cw) * ch, 128);

    stride_y_ = width;
    stride_u_ = cw;
    stride_v_ = cw;
}

void YuvFrame::Release() noexcept {
    plane_y_.clear();
    plane_u_.clear();
    plane_v_.clear();

    width_ = height_ = 0;
    stride_y_ = stride_u_ = stride_v_ = 0;
}

void YuvFrame::Fill(std::uint8_t y_val, std::uint8_t u_val, std::uint8_t v_val) noexcept {
    std::fill(plane_y_.begin(), plane_y_.end(), y_val);
    std::fill(plane_u_.begin(), plane_u_.end(), u_val);
    std::fill(plane_v_.begin(), plane_v_.end(), v_val);
}

void YuvFrame::SwapChroma() noexcept {
    std::swap(plane_u_, plane_v_);
    std::swap(stride_u_, stride_v_);
}