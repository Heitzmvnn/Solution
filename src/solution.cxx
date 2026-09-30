#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>

#include "bitmap_file.hxx"
#include "bitmap_to_yuv_frame_converter.hxx"
#include "cli_messages.hxx"
#include "exception_messages.hxx"
#include "image_overlay.hxx"
#include "yuv_file.hxx"
#include "yuv_frame.hxx"

using namespace CliMessages;
using namespace ExceptionMessages;

enum class ConversionMode {
    kSingleThread,
    kParallel,
    kSimd,
};

struct AppConfig {
    std::string yuv_in_path;
    std::string bmp_path;
    std::string yuv_out_path;
    std::int32_t width = 0;
    std::int32_t height = 0;
    std::int32_t overlay_x = 0;
    std::int32_t overlay_y = 0;
    std::int32_t num_threads = 0;
    ConversionMode mode = ConversionMode::kSingleThread;

    bool has_width = false;
    bool has_height = false;
    bool use_parallel = false;
    bool has_num_threads = false;
};

struct SplitArg {
    std::string name;
    std::string value;
    bool has_value = false;
};

std::int32_t ParseInt32(const std::string& str, const std::string& flag) {
    if (str.empty())
        throw std::invalid_argument(kEmptyValueFor + flag);

    char* end = nullptr;
    errno = 0;
    const std::int64_t value = std::strtoll(str.c_str(), &end, 10);
    if (errno != 0 || end == str.c_str() || *end != '\0')
        throw std::invalid_argument(kInvalidIntegerFor + flag + ": " + str);

    if (value < INT32_MIN || value > INT32_MAX)
        throw std::out_of_range(kValueOutOfRangeFor + flag + ": " + str);

    return static_cast<std::int32_t>(value);
}

ConversionMode ParseConversionMode(const std::string& str) {
    if (str == "single" || str == "single_thread" || str == "single-thread" ||
        str == "st")
        return ConversionMode::kSingleThread;

    if (str == "parallel" || str == "par" || str == "p")
        return ConversionMode::kParallel;

    if (str == "simd" || str == "s")
        return ConversionMode::kSimd;

    throw std::invalid_argument(kUnknownConversionMode + str + kUnknownConversionModeHint);
}

const char* ModeToString(ConversionMode mode) {
    switch (mode) {
    case ConversionMode::kSingleThread:
        return "single";

    case ConversionMode::kParallel:
        return "parallel";

    case ConversionMode::kSimd:
        return "simd";
    }

    return "unknown";
}

void PrintUsage() {
    std::cerr << kUsagePrefix << "solution" << kUsageMiddle << kUsageBody;
}

void PrintConfiguration(const AppConfig& config) {
    std::cout << "Configuration:\n"
        << "  yuv_in      : " << config.yuv_in_path << "\n"
        << "  bmp         : " << config.bmp_path << "\n"
        << "  yuv_out     : " << config.yuv_out_path << "\n"
        << "  width       : " << config.width << "\n"
        << "  height      : " << config.height << "\n"
        << "  x           : " << config.overlay_x << "\n"
        << "  y           : " << config.overlay_y << "\n"
        << "  mode        : " << ModeToString(config.mode) << "\n";

    if (config.mode == ConversionMode::kParallel)
    {
        if (config.has_num_threads) {
            if (config.num_threads == 0) {
                std::cout << "  num_threads : " << "auto" << "\n";
            }
            else {
                std::cout << "  num_threads : " << config.num_threads << "\n";
            }
        }
        else {
            std::cout << "  num_threads : " << "auto" << "\n";
        }
    }
}

SplitArg SplitFlag(const std::string& arg) {
    SplitArg result;
    const std::size_t eq_pos = arg.find('=');
    if (eq_pos == std::string::npos) {
        result.name = arg;
    }
    else {
        result.name = arg.substr(0, eq_pos);
        result.value = arg.substr(eq_pos + 1);
        result.has_value = true;
    }

    return result;
}

std::string RequireValue(const SplitArg& flag, std::int32_t argc, char** argv, std::int32_t* i) {
    if (flag.has_value)
        return flag.value;

    if (*i + 1 >= argc)
        throw std::invalid_argument(kMissingValueFor + flag.name);

    ++(*i);
    return argv[*i];
}

void Run(const AppConfig& config) {
    BitmapFile file(config.bmp_path);
    
    if (file.height() > config.height || file.width() > config.width)
        throw std::invalid_argument(kBitmapSizeExceedsYuv);

    BitmapToYuvFrameConverter converter(file);
    YuvFrame converted_bitmap;

    switch (config.mode) {
    case ConversionMode::kSingleThread:
        converted_bitmap = converter.Convert();
        break;

    case ConversionMode::kParallel:
        converted_bitmap = converter.ConvertParallel(config.num_threads);
        break;

    case ConversionMode::kSimd:
        converted_bitmap = converter.ConvertSimd();
        break;
    }

    YuvFile yuv_file(config.yuv_in_path, config.width, config.height);
    ImageOverlay overlay(converted_bitmap);
    overlay.ApplyToAll(yuv_file, config.overlay_x, config.overlay_y);
    yuv_file.Save(config.yuv_out_path);
}

int main(int argc, char** argv) {
    const std::int32_t argc32 = static_cast<std::int32_t>(argc);
    AppConfig config;

    if (argc32 == 1) {
        PrintUsage();
        return 0;
    }

    try {
        for (std::int32_t i = 1; i < argc32; ++i) {
            const std::string arg = argv[i];

            if (arg.empty() || arg[0] != '-')
                throw std::invalid_argument(kUnexpectedPositionalArgument + arg);

            const SplitArg flag = SplitFlag(arg);

            if (flag.name == "-h" || flag.name == "--help") {
                PrintUsage();
                return 0;
            }
            else if (flag.name == "-i" || flag.name == "--yuv-in") {
                config.yuv_in_path = RequireValue(flag, argc32, argv, &i);
            }
            else if (flag.name == "-b" || flag.name == "--bmp") {
                config.bmp_path = RequireValue(flag, argc32, argv, &i);
            }
            else if (flag.name == "-o" || flag.name == "--yuv-out") {
                config.yuv_out_path = RequireValue(flag, argc32, argv, &i);
            }
            else if (flag.name == "-w" || flag.name == "--width") {
                config.width = ParseInt32(RequireValue(flag, argc32, argv, &i), flag.name);
                config.has_width = true;
            }
            else if (flag.name == "-H" || flag.name == "--height") {
                config.height = ParseInt32(RequireValue(flag, argc32, argv, &i), flag.name);
                config.has_height = true;
            }
            else if (flag.name == "-x" || flag.name == "--x") {
                config.overlay_x = ParseInt32(RequireValue(flag, argc32, argv, &i), flag.name);
            }
            else if (flag.name == "-y" || flag.name == "--y") {
                config.overlay_y = ParseInt32(RequireValue(flag, argc32, argv, &i), flag.name);
            }
            else if (flag.name == "-m" || flag.name == "--mode") {
                config.mode = ParseConversionMode(RequireValue(flag, argc32, argv, &i));
                if (config.mode == ConversionMode::kParallel)
                    config.use_parallel = true;
            }
            else if (flag.name == "-j" || flag.name == "--threads") {
                config.num_threads = ParseInt32(RequireValue(flag, argc32, argv, &i), flag.name);
                config.has_num_threads = true;
            }
            else {
                throw std::invalid_argument(kUnknownOption + flag.name);
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << kArgumentParsingError << ex.what() << "\n\n";
        PrintUsage();
        return 0;
    }

    if (config.yuv_in_path.empty()) {
        std::cerr << kYuvInRequired;
        return 0;
    }

    if (config.bmp_path.empty()) {
        std::cerr << kBmpRequired;
        return 0;
    }

    if (config.yuv_out_path.empty()) {
        std::cerr << kYuvOutRequired;
        return 0;
    }

    if (!config.has_width) {
        std::cerr << kWidthRequired;
        return 0;
    }

    if (!config.has_height) {
        std::cerr << kHeightRequired;
        return 0;
    }

    if (config.width <= 0 || config.height <= 0) {
        std::cerr << kWidthHeightPositive;
        return 0;
    }

    if (!config.use_parallel && config.has_num_threads) {
        std::cerr << kThreadsOnlyForParallelMode;
        return 0;
    }

    PrintConfiguration(config);

    try {
        Run(config);
    }
    catch (const std::exception& ex) {
        std::cerr << kProcessingError << ex.what() << "\n";
    }

    return 0;
}