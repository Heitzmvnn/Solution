#ifndef CLI_MESSAGES_H_
#define CLI_MESSAGES_H_

namespace CliMessages {
	constexpr const char* kArgumentParsingError = "Argument parsing error: ";
	constexpr const char* kYuvInRequired = "Error: --yuv-in is required\n";
	constexpr const char* kBmpRequired = "Error: --bmp is required\n";
	constexpr const char* kYuvOutRequired = "Error: --yuv-out is required\n";
	constexpr const char* kWidthRequired = "Error: --width is required\n";
	constexpr const char* kHeightRequired = "Error: --height is required\n";
	constexpr const char* kWidthHeightPositive = "Error: width and height must be > 0\n";
	constexpr const char* kProcessingError = "Processing error: ";
	constexpr const char* kThreadsOnlyForParallelMode = "Error: number of threads can only be specified for parallel mode\n";
	constexpr const char* kUsagePrefix = "Usage:\n";
	constexpr const char* kUsageMiddle = " [options]\n\n";
	constexpr const char* kUsageBody =
		"Required options:\n"
		"  -i, --yuv-in  <path>  path to input YUV file\n"
		"  -b, --bmp     <path>  path to overlay BMP file\n"
		"  -o, --yuv-out <path>  path to output YUV file\n"
		"  -w, --width   <int>   width of YUV frames (pixels, > 0)\n"
		"  -H, --height  <int>   height of YUV frames (pixels, > 0)\n"
		"\n"
		"Optional options:\n"
		"  -x, --x       <int>   overlay X coordinate (default: 0, may be negative)\n"
		"  -y, --y       <int>   overlay Y coordinate (default: 0, may be negative)\n"
		"  -m, --mode    <mode>  conversion mode: single | parallel | simd"
		" (default: single)\n"
		"  -j, --threads <int>   number of threads for parallel mode (default: 0 = auto)\n"
		"\n"
		"  -h, --help            show this help and exit\n"
		"\n"
		"Both '--flag value' and '--flag=value' forms are accepted.\n";
}

#endif // CLI_MESSAGES_H_