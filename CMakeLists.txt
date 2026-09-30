cmake_minimum_required(VERSION 3.16)

project(solution
    VERSION 0.1.0
    DESCRIPTION "Overlays a BMP image onto a YUV video stream"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR
        "Only g++ and clang++ are supported. Detected: "
        "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        Debug Release RelWithDebInfo MinSizeRel)
endif()

include(CheckCXXSourceCompiles)
include(CMakePushCheckState)

set(YUV_ARCH_X86 FALSE)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|AMD64|amd64|i[3-6]86)")
    set(YUV_ARCH_X86 TRUE)
endif()

if(YUV_ARCH_X86)
    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "-mssse3")
    check_cxx_source_compiles("
        #include <tmmintrin.h>
        int main() {
            __m128i v = _mm_setzero_si128();
            __m128i r = _mm_shuffle_epi8(v, v);
            (void)r; return 0;
        }
    " YUV_COMPILER_HAS_SSSE3)
    cmake_pop_check_state()

    cmake_push_check_state(RESET)
    set(CMAKE_REQUIRED_FLAGS "-mavx2")
    check_cxx_source_compiles("
        #include <immintrin.h>
        int main() {
            __m256i v = _mm256_setzero_si256();
            (void)v; return 0;
        }
    " YUV_COMPILER_HAS_AVX2)
    cmake_pop_check_state()
endif()

if(NOT DEFINED YUV_COMPILER_HAS_SSSE3)
    set(YUV_COMPILER_HAS_SSSE3 FALSE)
endif()
if(NOT DEFINED YUV_COMPILER_HAS_AVX2)
    set(YUV_COMPILER_HAS_AVX2 FALSE)
endif()

option(SOLUTION_ENABLE_SIMD        "Enable x86 SIMD code paths"      ON)
option(SOLUTION_WARNINGS_AS_ERRORS "Treat warnings as errors"        OFF)

set(SOLUTION_SIMD_ENABLED FALSE)
if(SOLUTION_ENABLE_SIMD AND YUV_ARCH_X86 AND YUV_COMPILER_HAS_SSSE3)
    set(SOLUTION_SIMD_ENABLED TRUE)
endif()

message(STATUS "x86 arch:                ${YUV_ARCH_X86}")
message(STATUS "Compiler supports SSSE3: ${YUV_COMPILER_HAS_SSSE3}")
message(STATUS "Compiler supports AVX2:  ${YUV_COMPILER_HAS_AVX2}")
message(STATUS "SIMD enabled:            ${SOLUTION_SIMD_ENABLED}")

add_library(solution_warnings INTERFACE)

target_compile_options(solution_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wshadow -Wconversion
    $<$<BOOL:${SOLUTION_WARNINGS_AS_ERRORS}>:-Werror>
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(solution_warnings INTERFACE -Wno-psabi)
endif()

add_library(solution_simd_flags INTERFACE)

if(SOLUTION_SIMD_ENABLED)
    target_compile_definitions(solution_simd_flags INTERFACE
        YUV_HAVE_X86_SIMD=1
        "YUV_TARGET_SSSE3=__attribute__((target(\"ssse3\")))"
    )
    if(YUV_COMPILER_HAS_AVX2)
        target_compile_definitions(solution_simd_flags INTERFACE
            "YUV_TARGET_AVX2=__attribute__((target(\"avx2\")))"
        )
    else()
        target_compile_definitions(solution_simd_flags INTERFACE
            "YUV_TARGET_AVX2="
        )
    endif()
else()
    target_compile_definitions(solution_simd_flags INTERFACE
        YUV_HAVE_X86_SIMD=0
        "YUV_TARGET_SSSE3="
        "YUV_TARGET_AVX2="
    )
endif()

add_library(solution_core STATIC
    src/bitmap_file.cxx
    src/bitmap_to_yuv_frame_converter.cxx
    src/bitmap_to_yuv_frame_converter_simd.cxx
    src/image_overlay.cxx
    src/yuv_file.cxx
    src/yuv_frame.cxx
)

add_library(solution::core ALIAS solution_core)

target_include_directories(solution_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(solution_core
    PRIVATE solution_warnings
    PUBLIC  solution_simd_flags
)

if(SOLUTION_SIMD_ENABLED AND YUV_COMPILER_HAS_AVX2)
    set_source_files_properties(
        src/bitmap_to_yuv_frame_converter_simd.cxx
        PROPERTIES COMPILE_OPTIONS "-mavx2"
    )
else()
    message(STATUS
        "AVX2 branch in bitmap_to_yuv_frame_converter_simd.cxx is disabled "
        "(or the compiler does not support it)")
endif()

add_executable(solution src/solution.cxx)

target_link_libraries(solution PRIVATE solution::core)

include(GNUInstallDirs)

install(TARGETS solution solution_core
    EXPORT solutionTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)