#ifndef YUV_FRAME_H_
#define YUV_FRAME_H_

#include <cstddef>
#include <cstdint>
#include <vector>

class YuvFrame {
public:
    YuvFrame() = default;
    YuvFrame(std::int32_t width, std::int32_t height);

    std::int32_t width() const noexcept { return width_; }
    std::int32_t height() const noexcept { return height_; }

    std::int32_t chroma_width() const noexcept { return (width_ + 1) / 2; }
    std::int32_t chroma_height() const noexcept { return (height_ + 1) / 2; }
    
    std::int32_t stride_y() const noexcept { return stride_y_; }
    std::int32_t stride_u() const noexcept { return stride_u_; }
    std::int32_t stride_v() const noexcept { return stride_v_; }
    
    std::size_t plane_size_y() const noexcept { return plane_y_.size(); }
    std::size_t plane_size_u() const noexcept { return plane_u_.size(); }
    std::size_t plane_size_v() const noexcept { return plane_v_.size(); }
    
    std::vector<std::uint8_t>& plane_y() noexcept { return plane_y_; }
    std::vector<std::uint8_t>& plane_u() noexcept { return plane_u_; }
    std::vector<std::uint8_t>& plane_v() noexcept { return plane_v_; }
    const std::vector<std::uint8_t>& plane_y() const noexcept { return plane_y_; }
    const std::vector<std::uint8_t>& plane_u() const noexcept { return plane_u_; }
    const std::vector<std::uint8_t>& plane_v() const noexcept { return plane_v_; }
    
    std::uint8_t* data_y() noexcept { return plane_y_.data(); }
    std::uint8_t* data_u() noexcept { return plane_u_.data(); }
    std::uint8_t* data_v() noexcept { return plane_v_.data(); }
    const std::uint8_t* data_y() const noexcept { return plane_y_.data(); }
    const std::uint8_t* data_u() const noexcept { return plane_u_.data(); }
    const std::uint8_t* data_v() const noexcept { return plane_v_.data(); }
    
    std::uint8_t& y(std::int32_t x, std::int32_t row) noexcept {
        return plane_y_[static_cast<std::size_t>(row) * stride_y_ + x];
    }
    std::uint8_t& u(std::int32_t cx, std::int32_t cy) noexcept {
        return plane_u_[static_cast<std::size_t>(cy) * stride_u_ + cx];
    }
    std::uint8_t& v(std::int32_t cx, std::int32_t cy) noexcept {
        return plane_v_[static_cast<std::size_t>(cy) * stride_v_ + cx];
    }
    const std::uint8_t& y(std::int32_t x, std::int32_t row) const noexcept {
        return plane_y_[static_cast<std::size_t>(row) * stride_y_ + x];
    }
    const std::uint8_t& u(std::int32_t cx, std::int32_t cy) const noexcept {
        return plane_u_[static_cast<std::size_t>(cy) * stride_u_ + cx];
    }
    const std::uint8_t& v(std::int32_t cx, std::int32_t cy) const noexcept {
        return plane_v_[static_cast<std::size_t>(cy) * stride_v_ + cx];
    }
    
    bool empty() const noexcept { return plane_y_.empty(); }
    bool is_valid() const noexcept {
        return width_ > 0 && height_ > 0 &&
            !plane_y_.empty() && !plane_u_.empty() && !plane_v_.empty();
    }
    std::size_t size() const noexcept {
        return plane_y_.size() + plane_u_.size() + plane_v_.size();
    }
    
    void set_strides(std::int32_t stride_y, std::int32_t stride_u, std::int32_t stride_v) noexcept {
        stride_y_ = stride_y;
        stride_u_ = stride_u;
        stride_v_ = stride_v;
    }
    
    void Allocate(std::int32_t width, std::int32_t height);
    void Release() noexcept;
    void Fill(std::uint8_t y_val, std::uint8_t u_val, std::uint8_t v_val) noexcept;
    void SwapChroma() noexcept;

private:
    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::int32_t stride_y_ = 0;
    std::int32_t stride_u_ = 0;
    std::int32_t stride_v_ = 0;

    std::vector<std::uint8_t> plane_y_;
    std::vector<std::uint8_t> plane_u_;
    std::vector<std::uint8_t> plane_v_;
};

#endif // YUV_FRAME_H_