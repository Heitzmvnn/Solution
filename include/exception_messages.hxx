#ifndef EXCEPTION_MESSAGES_H_
#define EXCEPTION_MESSAGES_H_

namespace ExceptionMessages {
	constexpr const char* kEmptyValueFor = "Empty value for ";
	constexpr const char* kInvalidIntegerFor = "Invalid integer for ";
	constexpr const char* kValueOutOfRangeFor = "Value out of range for ";
	constexpr const char* kMissingValueFor = "Missing value for ";
	constexpr const char* kUnexpectedPositionalArgument = "Unexpected positional argument: ";
	constexpr const char* kUnknownOption = "Unknown option: ";
	constexpr const char* kUnknownConversionMode = "Unknown conversion mode: '";
	constexpr const char* kUnknownConversionModeHint = "'. Expected one of: single, parallel, simd";
	constexpr const char* kBitmapSizeExceedsYuv = "Bitmap image size must not exceed the YUV video size.";
	constexpr const char* kFailedToOpenFile = "Failed to open file: ";
	constexpr const char* kFailedToReadBmpHeader = "Failed to read BitmapHeader";
	constexpr const char* kFileIsNotBmpMissingBmSignature = "File is not a BMP: missing BM signature";
	constexpr const char* kFailedToReadBmpInfoHeader = "Failed to read BitmapInfoHeader";
	constexpr const char* kOnlyBmpInfoHeader40BytesSupported = "Only BitMapInfoHeader of size 40 bytes is supported";
	constexpr const char* kOnlySinglePlaneBmpSupported = "Only BMP with a single color plane (biPlanes == 1) is supported.";
	constexpr const char* kOnly24BitBmpSupported = "Only 24-bit BMP is supported";
	constexpr const char* kOnlyUncompressedBmpBiRgbSupported = "Only uncompressed BMP BI_RGB is supported";
	constexpr const char* kInvalidBmpDimensions = "Invalid BMP dimensions";
	constexpr const char* kInvalidDataOffsetInBmp = "Invalid dataOffset in BMP";
	constexpr const char* kDataOffsetOverlapsHeaders = "Invalid BMP: dataOffset points inside the file headers.";
	constexpr const char* kFileSizeSmallerThanDataOffset = "Invalid BMP: fileSize is smaller than dataOffset.";
	constexpr const char* kTruncatedBmpPixelData = "Invalid BMP: file is truncated before the declared pixel data ends.";
	constexpr const char* kFailedToReadBmpPixelRow = "Failed to read BMP pixel row";
	constexpr const char* kInvalidDimensions = "Invalid dimensions";
	constexpr const char* kFileIsEmpty = "File is empty: ";
	constexpr const char* kFileSizeNotMultipleOfFrameSize = "File size is not a multiple of frame size";
	constexpr const char* kFailedToReadFrameData = "Failed to read frame data";
	constexpr const char* kFrameIndexOutOfRange = "Frame index out of range";
	constexpr const char* kAddFrameSizeMismatch = "Frame size mismatch";
	constexpr const char* kWriteFrameFailedToWriteYPlane = "Failed to write Y plane";
	constexpr const char* kWriteFrameFailedToWriteUPlane = "Failed to write U plane";
	constexpr const char* kWriteFrameFailedToWriteVPlane = "Failed to write V plane";
	constexpr const char* kSaveNoFramesToSave = "No frames to save";
	constexpr const char* kSaveFailedToCloseFile = "Failed to close file";
	constexpr const char* kSaveFrameInvalidFrame = "Invalid frame";
	constexpr const char* kInvalidOverlayFrame = "Invalid overlay frame";
	constexpr const char* kOverlayDimensionsMustBeEven = "Overlay dimensions must be even";
	constexpr const char* kPositionMustBeEven = "Position must be even (chroma alignment)";
	constexpr const char* kInvalidTargetFrame = "Invalid target frame";
	constexpr const char* kInvalidBmpSize = "Invalid bitmap size";
	constexpr const char* kWidthAndHeightMustBeEvenForI420 = "Width and height must be even for I420";
	constexpr const char* kBmpPixelBufferTooSmall = "Bitmap pixel buffer too small";
}

#endif // EXCEPTION_MESSAGES_H_