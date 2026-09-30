#include "bitmap_to_yuv_frame_converter.hxx"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#if YUV_HAVE_X86_SIMD
#include <immintrin.h>
#endif

void BitmapToYuvFrameConverter::RgbToYuvRowsScalar(std::int32_t y_begin, std::int32_t y_end) {
    const std::int32_t w = width_;
    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::int32_t stride_y = frame_.stride_y();

    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const PixelBgr* src_row = bgr.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w);

        std::uint8_t* y_dst = frame_.data_y() + static_cast<std::size_t>(y) * stride_y;
        std::uint8_t* u_dst = u444_.data() + static_cast<std::size_t>(y) * w;
        std::uint8_t* v_dst = v444_.data() + static_cast<std::size_t>(y) * w;

        for (std::int32_t x = 0; x < w; ++x) {
            std::uint8_t yy = 0, uu = 0, vv = 0;
            RgbToYuv(src_row[x].r, src_row[x].g, src_row[x].b, yy, uu, vv);

            y_dst[x] = yy;
            u_dst[x] = uu;
            v_dst[x] = vv;
        }
    }
}

void BitmapToYuvFrameConverter::RgbToYuvRowsSimd(std::int32_t y_begin, std::int32_t y_end) {
#if YUV_HAVE_X86_SIMD
#if defined(__AVX2__)
    if (CpuHasAvx2()) {
        RgbToYuvRowsAvx2(y_begin, y_end);
        return;
    }
#endif // __AVX2__
    RgbToYuvRowsSsse3(y_begin, y_end);
#else
    RgbToYuvRowsScalar(y_begin, y_end);
#endif // YUV_HAVE_X86_SIMD
}

#if YUV_HAVE_X86_SIMD

YUV_TARGET_SSSE3
void BitmapToYuvFrameConverter::RgbToYuvRowsSsse3(std::int32_t y_begin, std::int32_t y_end) {
    const std::int32_t w = width_;
    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::int32_t stride_y = frame_.stride_y();

    const __m128i shuf_b = _mm_setr_epi8(0, 3, 6, 9, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_g = _mm_setr_epi8(1, 4, 7, 10, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1);
    const __m128i shuf_r = _mm_setr_epi8(2, 5, 8, 11, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1);

    const __m128i zero = _mm_setzero_si128();

    const __m128i coeff_rg_y = _mm_setr_epi16(77, 150, 77, 150, 77, 150, 77, 150);
    const __m128i coeff_b_y = _mm_setr_epi16(29, 0, 29, 0, 29, 0, 29, 0);
    const __m128i coeff_rg_u = _mm_setr_epi16(-43, -85, -43, -85, -43, -85, -43, -85);
    const __m128i coeff_b_u = _mm_setr_epi16(128, 0, 128, 0, 128, 0, 128, 0);
    const __m128i coeff_rg_v = _mm_setr_epi16(128, -107, 128, -107, 128, -107, 128, -107);
    const __m128i coeff_b_v = _mm_setr_epi16(-21, 0, -21, 0, -21, 0, -21, 0);

    const __m128i c128 = _mm_set1_epi32(128);

    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const std::uint8_t* src_row = reinterpret_cast<const std::uint8_t*>(
            bgr.data() + static_cast<std::size_t>(y) * w);

        std::uint8_t* y_dst = frame_.data_y() + static_cast<std::size_t>(y) * stride_y;
        std::uint8_t* u_dst = u444_.data() + static_cast<std::size_t>(y) * w;
        std::uint8_t* v_dst = v444_.data() + static_cast<std::size_t>(y) * w;

        std::int32_t x = 0;
        for (; x + 6 <= w; x += 4) {
            __m128i packed = _mm_loadu_si128(
                reinterpret_cast<const __m128i*>(src_row + x * 3));

            __m128i b8 = _mm_shuffle_epi8(packed, shuf_b);
            __m128i g8 = _mm_shuffle_epi8(packed, shuf_g);
            __m128i r8 = _mm_shuffle_epi8(packed, shuf_r);

            __m128i b16 = _mm_unpacklo_epi8(b8, zero);
            __m128i g16 = _mm_unpacklo_epi8(g8, zero);
            __m128i r16 = _mm_unpacklo_epi8(r8, zero);

            __m128i rg = _mm_unpacklo_epi16(r16, g16);
            __m128i bz = _mm_unpacklo_epi16(b16, zero);

            __m128i y_sum = _mm_add_epi32(_mm_madd_epi16(rg, coeff_rg_y),
                _mm_madd_epi16(bz, coeff_b_y));
            __m128i u_sum = _mm_add_epi32(_mm_madd_epi16(rg, coeff_rg_u),
                _mm_madd_epi16(bz, coeff_b_u));
            __m128i v_sum = _mm_add_epi32(_mm_madd_epi16(rg, coeff_rg_v),
                _mm_madd_epi16(bz, coeff_b_v));

            y_sum = _mm_srai_epi32(y_sum, 8);
            u_sum = _mm_add_epi32(_mm_srai_epi32(u_sum, 8), c128);
            v_sum = _mm_add_epi32(_mm_srai_epi32(v_sum, 8), c128);

            __m128i y8 = _mm_packus_epi16(_mm_packs_epi32(y_sum, zero), zero);
            __m128i u8 = _mm_packus_epi16(_mm_packs_epi32(u_sum, zero), zero);
            __m128i v8 = _mm_packus_epi16(_mm_packs_epi32(v_sum, zero), zero);

            std::uint32_t y_val = static_cast<std::uint32_t>(_mm_cvtsi128_si32(y8));
            std::uint32_t u_val = static_cast<std::uint32_t>(_mm_cvtsi128_si32(u8));
            std::uint32_t v_val = static_cast<std::uint32_t>(_mm_cvtsi128_si32(v8));

            std::memcpy(y_dst + x, &y_val, 4);
            std::memcpy(u_dst + x, &u_val, 4);
            std::memcpy(v_dst + x, &v_val, 4);
        }

        for (; x < w; ++x) {
            const PixelBgr& p = bgr[static_cast<std::size_t>(y) * w + x];
            std::uint8_t yy, uu, vv;
            RgbToYuv(p.r, p.g, p.b, yy, uu, vv);
            y_dst[x] = yy;
            u_dst[x] = uu;
            v_dst[x] = vv;
        }
    }
}

#if defined(__AVX2__)

YUV_TARGET_AVX2
void BitmapToYuvFrameConverter::RgbToYuvRowsAvx2(std::int32_t y_begin, std::int32_t y_end) {
    const std::int32_t w = width_;
    const std::vector<PixelBgr>& bgr = bitmap_file_.data();
    const std::int32_t stride_y = frame_.stride_y();

    const __m256i shuf_b = _mm256_setr_epi8(
        0, 3, 6, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        0, 3, 6, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m256i shuf_g = _mm256_setr_epi8(
        1, 4, 7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        1, 4, 7, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);
    const __m256i shuf_r = _mm256_setr_epi8(
        2, 5, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        2, 5, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1);

    const __m256i zero = _mm256_setzero_si256();

    const __m256i coeff_rg_y = _mm256_setr_epi16(
        77, 150, 77, 150, 77, 150, 77, 150,
        77, 150, 77, 150, 77, 150, 77, 150);
    const __m256i coeff_b_y = _mm256_setr_epi16(
        29, 0, 29, 0, 29, 0, 29, 0,
        29, 0, 29, 0, 29, 0, 29, 0);
    const __m256i coeff_rg_u = _mm256_setr_epi16(
        -43, -85, -43, -85, -43, -85, -43, -85,
        -43, -85, -43, -85, -43, -85, -43, -85);
    const __m256i coeff_b_u = _mm256_setr_epi16(
        128, 0, 128, 0, 128, 0, 128, 0,
        128, 0, 128, 0, 128, 0, 128, 0);
    const __m256i coeff_rg_v = _mm256_setr_epi16(
        128, -107, 128, -107, 128, -107, 128, -107,
        128, -107, 128, -107, 128, -107, 128, -107);
    const __m256i coeff_b_v = _mm256_setr_epi16(
        -21, 0, -21, 0, -21, 0, -21, 0,
        -21, 0, -21, 0, -21, 0, -21, 0);

    const __m256i c128 = _mm256_set1_epi32(128);

    for (std::int32_t y = y_begin; y < y_end; ++y) {
        const std::uint8_t* src_row = reinterpret_cast<const std::uint8_t*>(
            bgr.data() + static_cast<std::size_t>(y) * w);

        std::uint8_t* y_dst = frame_.data_y() + static_cast<std::size_t>(y) * stride_y;
        std::uint8_t* u_dst = u444_.data() + static_cast<std::size_t>(y) * w;
        std::uint8_t* v_dst = v444_.data() + static_cast<std::size_t>(y) * w;

        std::int32_t x = 0;
        for (; x + 12 <= w; x += 8) {
            __m256i packed = _mm256_loadu2_m128i(
                reinterpret_cast<const __m128i*>(src_row + (x + 4) * 3),
                reinterpret_cast<const __m128i*>(src_row + x * 3));

            __m256i b8 = _mm256_shuffle_epi8(packed, shuf_b);
            __m256i g8 = _mm256_shuffle_epi8(packed, shuf_g);
            __m256i r8 = _mm256_shuffle_epi8(packed, shuf_r);

            __m256i b16 = _mm256_unpacklo_epi8(b8, zero);
            __m256i g16 = _mm256_unpacklo_epi8(g8, zero);
            __m256i r16 = _mm256_unpacklo_epi8(r8, zero);

            __m256i rg = _mm256_unpacklo_epi16(r16, g16);
            __m256i bz = _mm256_unpacklo_epi16(b16, zero);

            __m256i y_sum = _mm256_add_epi32(_mm256_madd_epi16(rg, coeff_rg_y),
                _mm256_madd_epi16(bz, coeff_b_y));
            __m256i u_sum = _mm256_add_epi32(_mm256_madd_epi16(rg, coeff_rg_u),
                _mm256_madd_epi16(bz, coeff_b_u));
            __m256i v_sum = _mm256_add_epi32(_mm256_madd_epi16(rg, coeff_rg_v),
                _mm256_madd_epi16(bz, coeff_b_v));

            y_sum = _mm256_srai_epi32(y_sum, 8);
            u_sum = _mm256_add_epi32(_mm256_srai_epi32(u_sum, 8), c128);
            v_sum = _mm256_add_epi32(_mm256_srai_epi32(v_sum, 8), c128);

            __m256i y8 = _mm256_packus_epi16(_mm256_packs_epi32(y_sum, zero), zero);
            __m256i u8 = _mm256_packus_epi16(_mm256_packs_epi32(u_sum, zero), zero);
            __m256i v8 = _mm256_packus_epi16(_mm256_packs_epi32(v_sum, zero), zero);

            const std::uint32_t y_lo = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_castsi256_si128(y8)));
            const std::uint32_t y_hi = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_extracti128_si256(y8, 1)));
            const std::uint32_t u_lo = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_castsi256_si128(u8)));
            const std::uint32_t u_hi = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_extracti128_si256(u8, 1)));
            const std::uint32_t v_lo = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_castsi256_si128(v8)));
            const std::uint32_t v_hi = static_cast<std::uint32_t>(
                _mm_cvtsi128_si32(_mm256_extracti128_si256(v8, 1)));

            std::memcpy(y_dst + x, &y_lo, 4);
            std::memcpy(y_dst + x + 4, &y_hi, 4);
            std::memcpy(u_dst + x, &u_lo, 4);
            std::memcpy(u_dst + x + 4, &u_hi, 4);
            std::memcpy(v_dst + x, &v_lo, 4);
            std::memcpy(v_dst + x + 4, &v_hi, 4);
        }

        for (; x < w; ++x) {
            const PixelBgr& p = bgr[static_cast<std::size_t>(y) * w + x];
            std::uint8_t yy, uu, vv;
            RgbToYuv(p.r, p.g, p.b, yy, uu, vv);
            y_dst[x] = yy;
            u_dst[x] = uu;
            v_dst[x] = vv;
        }
    }
}

#endif // __AVX2__

bool BitmapToYuvFrameConverter::CpuHasAvx2() noexcept {
#if defined(__GNUC__) || defined(__clang__)
    __builtin_cpu_init();
    return __builtin_cpu_supports("avx2");
#elif defined(_MSC_VER)
    int info[4] = { 0, 0, 0, 0 };
    __cpuid(info, 0);
    if (info[0] < 7) return false;
    __cpuidex(info, 7, 0);
    return (info[1] & (1 << 5)) != 0;
#else
    return false;
#endif
}

#endif // YUV_HAVE_X86_SIMD