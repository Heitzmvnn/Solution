#ifndef YUV_FILE_H_
#define YUV_FILE_H_

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

#include "yuv_frame.hxx"

class YuvFile {
public:
    YuvFile(const std::string& file_path, std::int32_t width, std::int32_t height);

    std::int32_t width() const noexcept { return width_; }
    std::int32_t height() const noexcept { return height_; }
    std::size_t frame_count() const noexcept { return frames_.size(); }
    bool empty() const noexcept { return frames_.empty(); }

    YuvFrame& frame(std::size_t index);
    const YuvFrame& frame(std::size_t index) const;

    std::vector<YuvFrame>& frames() noexcept { return frames_; }
    const std::vector<YuvFrame>& frames() const noexcept { return frames_; }

    void AddFrame(YuvFrame frame);
    void Clear() noexcept { frames_.clear(); }
    void Save(const std::string& file_path) const;
    static void SaveFrame(const YuvFrame& frame, const std::string& file_path, bool append = false);

private:
    void LoadFromFile(const std::string& file_path);
    static void WriteFrame(std::ostream& out, const YuvFrame& frame);

    std::int32_t width_ = 0;
    std::int32_t height_ = 0;
    std::vector<YuvFrame> frames_;
};

#endif // YUV_FILE_H_