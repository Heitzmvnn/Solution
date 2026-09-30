#include "yuv_file.hxx"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <utility>

#include "exception_messages.hxx"

using namespace ExceptionMessages;

YuvFile::YuvFile(const std::string& file_path, std::int32_t width, std::int32_t height)
    : width_(width), height_(height) {
    if (width_ <= 0 || height_ <= 0)
        throw std::runtime_error(kInvalidDimensions);

    LoadFromFile(file_path);
}

void YuvFile::LoadFromFile(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file)
        throw std::runtime_error(kFailedToOpenFile + file_path);

    const std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (file_size <= 0)
        throw std::runtime_error(kFileIsEmpty + file_path);

    const std::size_t w = static_cast<std::size_t>(width_);
    const std::size_t h = static_cast<std::size_t>(height_);

    const std::size_t y_size = w * h;
    const std::size_t cw = w / 2;
    const std::size_t ch = h / 2;
    const std::size_t c_size = cw * ch;
    const std::size_t frame_size = y_size + 2 * c_size;

    if (static_cast<std::size_t>(file_size) % frame_size != 0)
        throw std::runtime_error(kFileSizeNotMultipleOfFrameSize);

    const std::size_t num_frames = static_cast<std::size_t>(file_size) / frame_size;

    frames_.clear();
    frames_.reserve(num_frames);

    for (std::size_t i = 0; i < num_frames; ++i) {
        YuvFrame frame(width_, height_);

        file.read(reinterpret_cast<char*>(frame.data_y()), static_cast<std::streamsize>(y_size));
        file.read(reinterpret_cast<char*>(frame.data_u()), static_cast<std::streamsize>(c_size));
        file.read(reinterpret_cast<char*>(frame.data_v()), static_cast<std::streamsize>(c_size));

        if (!file)
            throw std::runtime_error(kFailedToReadFrameData);

        frames_.push_back(std::move(frame));
    }
}

YuvFrame& YuvFile::frame(std::size_t index) {
    if (index >= frames_.size())
        throw std::out_of_range(kFrameIndexOutOfRange);

    return frames_[index];
}

const YuvFrame& YuvFile::frame(std::size_t index) const {
    if (index >= frames_.size())
        throw std::out_of_range(kFrameIndexOutOfRange);

    return frames_[index];
}

void YuvFile::AddFrame(YuvFrame frame) {
    if (width_ == 0 && height_ == 0) {
        width_ = frame.width();
        height_ = frame.height();
    }
    else if (frame.width() != width_ || frame.height() != height_) {
        throw std::runtime_error(kAddFrameSizeMismatch);
    }

    frames_.push_back(std::move(frame));
}

void YuvFile::WriteFrame(std::ostream& out, const YuvFrame& frame) {
    const std::int32_t w = frame.width();
    const std::int32_t h = frame.height();
    const std::int32_t cw = frame.chroma_width();
    const std::int32_t ch = frame.chroma_height();

    const std::int32_t stride_y = frame.stride_y();
    const std::int32_t stride_u = frame.stride_u();
    const std::int32_t stride_v = frame.stride_v();

    const std::uint8_t* y_data = frame.data_y();
    for (std::int32_t y = 0; y < h; ++y) {
        out.write(reinterpret_cast<const char*>(y_data + y * stride_y), static_cast<std::streamsize>(w));
        if (!out)
            throw std::runtime_error(kWriteFrameFailedToWriteYPlane);
    }

    const std::uint8_t* u_data = frame.data_u();
    for (std::int32_t y = 0; y < ch; ++y) {
        out.write(reinterpret_cast<const char*>(u_data + y * stride_u), static_cast<std::streamsize>(cw));
        if (!out)
            throw std::runtime_error(kWriteFrameFailedToWriteUPlane);
    }

    const std::uint8_t* v_data = frame.data_v();
    for (std::int32_t y = 0; y < ch; ++y) {
        out.write(reinterpret_cast<const char*>(v_data + y * stride_v), static_cast<std::streamsize>(cw));
        if (!out)
            throw std::runtime_error(kWriteFrameFailedToWriteVPlane);
    }
}

void YuvFile::Save(const std::string& file_path) const {
    if (frames_.empty())
        throw std::runtime_error(kSaveNoFramesToSave);

    std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
    if (!file)
        throw std::runtime_error(kFailedToOpenFile + file_path);

    for (const YuvFrame& f : frames_)
        WriteFrame(file, f);

    file.close();
    if (!file)
        throw std::runtime_error(kSaveFailedToCloseFile);
}

void YuvFile::SaveFrame(const YuvFrame& frame, const std::string& file_path, bool append) {
    if (!frame.is_valid())
        throw std::runtime_error(kSaveFrameInvalidFrame);

    const std::ios::openmode mode = std::ios::binary | (append ? std::ios::app : std::ios::trunc);
    std::ofstream file(file_path, mode);
    if (!file)
        throw std::runtime_error(kFailedToOpenFile + file_path);

    WriteFrame(file, frame);

    file.close();
    if (!file)
        throw std::runtime_error(kSaveFailedToCloseFile);
}