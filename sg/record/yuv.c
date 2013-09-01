/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#include "sg/record.h"
#include <emmintrin.h>
#include <string.h>

/*
  Y' =  0.299    R + 0.587    G + 0.114    B
  PB = -0.168736 R - 0.331264 G + 0.5      B
  PR =  0.5      R - 0.418688 G - 0.081312 B

  >>> def convert(a, n):
  ...     return [round(x * 2**n * 219 / 255) for x in a]
  >>> convert([0.299, 0.587, 0.114], 16)
  [16829, 33039, 6416]
  >>> convert([-0.168736, -0.331264, 0.5], 16)
  [-9497, 18645, 28142]
  >>> convert([0.5, -0.418688, -0.081312], 16)
  [28142, -23565, -4577]
*/
__attribute__((aligned(16)))
static const unsigned short SG_YUV_CONST[5][8] = {
    { 16829, 33039, 6416, 0, 16829, 33039, 6416, 0 },
    { 0x1080, 0x1080, 0x1080, 0x1080, 0x1080, 0x1080, 0x1080, 0x1080 },

    { -9497, -18645, 28142, 0, -9497, -18645, 28142, 0 },
    { 28142, -23565, -4577, 0, 28142, -23565, -4577, 0 },
    { 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080 }
};

/* ======================================== */

/* Given: (AAAA BBBB) (CCCC DDDD)
   Compute: (A B C D A B C D) */
static inline __m128i
sse_mix1(__m128i x, __m128i y)
{
    __m128i a, b, c, d;
    a = _mm_unpacklo_epi16(x, y);
    b = _mm_unpackhi_epi16(x, y);
    c = _mm_unpacklo_epi16(a, b);
    d = _mm_unpackhi_epi16(a, b);
    return _mm_add_epi16(c, d);
}

/* Given: (aaaa bbbb cccc dddd)
   Compute: (A B C D A B C D), multiplied by coeff */
static inline __m128i
sse_mul1u(__m128i data, __m128i coeff)
{
    __m128i zero = _mm_set1_epi32(0);
    return sse_mix1(
        _mm_mulhi_epu16(_mm_unpacklo_epi8(zero, data), coeff),
        _mm_mulhi_epu16(_mm_unpackhi_epi8(zero, data), coeff));
}

/* Given: (A B C D A B C D) (E F G H E F G H)
   Compute: (A B C D E F G H) */
static inline __m128i
sse_mix2(__m128i x, __m128i y)
{
    return _mm_add_epi16(
        _mm_unpacklo_epi64(x, y),
        _mm_unpackhi_epi64(x, y));
}

/* Given: (A B C D E F G H) (I J K L M N O P)
   Compute: (A B C D E F G H I J K L M N O P) */
static inline __m128i
sse_pack(__m128i x, __m128i y, __m128i offset)
{
    return
        _mm_packus_epi16(
            _mm_srli_epi16(_mm_add_epi16(x, offset), 8),
            _mm_srli_epi16(_mm_add_epi16(y, offset), 8));
}

/* ======================================== */

static inline __m128i
sse_mul1s(__m128i data, __m128i coeff)
{
    __m128i zero = _mm_set1_epi32(0);
    return sse_mix2(
        _mm_mulhi_epi16(
            _mm_srli_epi16(_mm_unpacklo_epi8(zero, data), 2),
            coeff),
        _mm_mulhi_epi16(
            _mm_srli_epi16(_mm_unpackhi_epi8(zero, data), 2),
            coeff));
}

/* ======================================== */

void
sg_record_yuv_from_rgb(void *dest, const void *src,
                       int width, int height)
{
    const __m128i *SG_RESTRICT in = src;
    __m128i *SG_RESTRICT out;
    __m128i coeff_y, coeff_pb, coeff_pr, off_y, off_pbr, coeff;
    __m128i a, b, c, d, e, f;
    int x, y, yy, nx = width/32, ny = height/2, i;

    coeff_y  = *(const __m128i *) SG_YUV_CONST[0];
    off_y    = *(const __m128i *) SG_YUV_CONST[1];
    coeff_pb = *(const __m128i *) SG_YUV_CONST[2];
    coeff_pr = *(const __m128i *) SG_YUV_CONST[3];
    off_pbr  = *(const __m128i *) SG_YUV_CONST[4];

    for (y = 0; y < ny; ++y) {
        out = dest;
        for (yy = y*2; yy < y*2 + 2; ++yy) {
            for (x = 0; x < 2*nx; ++x) {
                a = sse_mul1u(in[yy * (8*nx) + 4*x + 0], coeff_y);
                b = sse_mul1u(in[yy * (8*nx) + 4*x + 1], coeff_y);
                c = sse_mix2(a, b);
                a = sse_mul1u(in[yy * (8*nx) + 4*x + 2], coeff_y);
                b = sse_mul1u(in[yy * (8*nx) + 4*x + 3], coeff_y);
                d = sse_mix2(a, b);
                out[(2*ny - 1 - yy)*2*nx+x] = sse_pack(c, d, off_y);
            }
        }
        for (i = 0; i < 2; ++i) {
            out = (__m128i *) dest + nx * ny * (4 + i);
            coeff = i == 0 ? coeff_pb : coeff_pr;
            for (x = 0; x < nx; ++x) {
                yy = 2*y;

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 0], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 0], coeff);
                c = _mm_add_epi16(a, b);
                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 1], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 1], coeff);
                d = _mm_add_epi16(a, b);
                c = sse_mix1(c, d);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 2], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 2], coeff);
                d = _mm_add_epi16(a, b);
                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 3], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 3], coeff);
                e = _mm_add_epi16(a, b);
                d = sse_mix1(d, e);

                c = sse_mix2(c, d);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 4], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 4], coeff);
                d = _mm_add_epi16(a, b);
                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 5], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 5], coeff);
                e = _mm_add_epi16(a, b);
                d = sse_mix1(d, e);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 6], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 6], coeff);
                e = _mm_add_epi16(a, b);
                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 7], coeff);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 7], coeff);
                f = _mm_add_epi16(a, b);
                e = sse_mix1(e, f);

                d = sse_mix2(d, e);

                out[(ny - 1 - y)*nx+x] = sse_pack(c, d, off_pbr);
            }
        }
    }
}
