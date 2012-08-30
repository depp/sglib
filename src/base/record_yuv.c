#include "defs.h"
#include "record.h"
#include <string.h>

typedef char __attribute__((vector_size(16))) v16qi;
typedef short __attribute__((vector_size(16))) v8hi;
typedef float __attribute__((vector_size(16))) v4sf;
typedef long long __attribute__((vector_size(16))) v2di;

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
static inline v8hi
sse_mix1(v8hi x, v8hi y)
{
    v8hi a, b, c, d;
    a = __builtin_ia32_punpcklwd128(x, y);
    b = __builtin_ia32_punpckhwd128(x, y);
    c = __builtin_ia32_punpcklwd128(a, b);
    d = __builtin_ia32_punpckhwd128(a, b);
    return __builtin_ia32_paddw128(c, d);
}

/* Given: (aaaa bbbb cccc dddd)
   Compute: (A B C D A B C D), multiplied by coeff */
static inline v8hi
sse_mul1u(v16qi data, v8hi coeff, v16qi zero)
{
    return sse_mix1(
        __builtin_ia32_pmulhuw128(
            (v8hi) __builtin_ia32_punpcklbw128(zero, data),
            coeff),
        __builtin_ia32_pmulhuw128(
            (v8hi) __builtin_ia32_punpckhbw128(zero, data),
            coeff));
}

/* Given: (A B C D A B C D) (E F G H E F G H)
   Compute: (A B C D E F G H) */
static inline v8hi
sse_mix2(v8hi x, v8hi y)
{
    return __builtin_ia32_paddw128(
        (v8hi) __builtin_ia32_movlhps((v4sf) x, (v4sf) y),
        (v8hi) __builtin_ia32_movhlps((v4sf) y, (v4sf) x));
}

/* Given: (A B C D E F G H) (I J K L M N O P)
   Compute: (A B C D E F G H I J K L M N O P) */
static inline v16qi
sse_pack(v8hi x, v8hi y, v8hi offset)
{
    return
        __builtin_ia32_packuswb128(
            __builtin_ia32_psrlwi128(__builtin_ia32_paddw128(x, offset), 8),
            __builtin_ia32_psrlwi128(__builtin_ia32_paddw128(y, offset), 8));
}

/* ======================================== */

static inline v8hi
sse_mul1s(v16qi data, v8hi coeff, v16qi zero)
{
    return sse_mix2(
        __builtin_ia32_pmulhw128(
            __builtin_ia32_psrlwi128(
                (v8hi) __builtin_ia32_punpcklbw128(zero, data), 2),
            coeff),
        __builtin_ia32_pmulhw128(
            __builtin_ia32_psrlwi128(
                (v8hi) __builtin_ia32_punpckhbw128(zero, data), 2),
            coeff));
}

/* ======================================== */

void
sg_record_yuv_from_rgb(void *dest, const void *src,
                       int width, int height)
{
    const v16qi *SG_RESTRICT in = src;
    v16qi *SG_RESTRICT out;
    v16qi zero;
    v8hi coeff_y, coeff_pb, coeff_pr, off_y, off_pbr, coeff;
    v8hi a, b, c, d, e, f;
    int x, y, yy, nx = width/32, ny = height/2, i;

    coeff_y  = *(const v8hi *) SG_YUV_CONST[0];
    off_y    = *(const v8hi *) SG_YUV_CONST[1];
    coeff_pb = *(const v8hi *) SG_YUV_CONST[2];
    coeff_pr = *(const v8hi *) SG_YUV_CONST[3];
    off_pbr  = *(const v8hi *) SG_YUV_CONST[4];
    zero = (v16qi) __builtin_ia32_pandn128((v2di) coeff_y, (v2di) coeff_y);

    for (y = 0; y < ny; ++y) {
        out = dest;
        for (yy = y*2; yy < y*2 + 2; ++yy) {
            for (x = 0; x < 2*nx; ++x) {
                a = sse_mul1u(in[yy * (8*nx) + 4*x + 0], coeff_y, zero);
                b = sse_mul1u(in[yy * (8*nx) + 4*x + 1], coeff_y, zero);
                c = sse_mix2(a, b);
                a = sse_mul1u(in[yy * (8*nx) + 4*x + 2], coeff_y, zero);
                b = sse_mul1u(in[yy * (8*nx) + 4*x + 3], coeff_y, zero);
                d = sse_mix2(a, b);
                out[(2*ny - 1 - yy)*2*nx+x] = sse_pack(c, d, off_y);
            }
        }
        for (i = 0; i < 2; ++i) {
            out = (v16qi *) dest + nx * ny * (4 + i);
            coeff = i == 0 ? coeff_pb : coeff_pr;
            for (x = 0; x < nx; ++x) {
                yy = 2*y;

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 0], coeff, zero);
                b = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 1], coeff, zero);
                c = sse_mix1(a, b);
                a = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 0], coeff, zero);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 1], coeff, zero);
                d = sse_mix1(a, b);
                c = __builtin_ia32_paddw128(c, d);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 2], coeff, zero);
                b = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 3], coeff, zero);
                d = sse_mix1(a, b);
                a = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 2], coeff, zero);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 3], coeff, zero);
                e = sse_mix1(a, b);
                d = __builtin_ia32_paddw128(d, e);

                c = sse_mix2(c, d);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 4], coeff, zero);
                b = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 5], coeff, zero);
                d = sse_mix1(a, b);
                a = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 4], coeff, zero);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 5], coeff, zero);
                e = sse_mix1(a, b);
                d = __builtin_ia32_paddw128(d, e);

                a = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 6], coeff, zero);
                b = sse_mul1s(in[(yy+0) * (8*nx) + 8*x + 7], coeff, zero);
                e = sse_mix1(a, b);
                a = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 6], coeff, zero);
                b = sse_mul1s(in[(yy+1) * (8*nx) + 8*x + 7], coeff, zero);
                f = sse_mix1(a, b);
                e = __builtin_ia32_paddw128(e, f);

                d = sse_mix2(d, e);

                out[(ny - 1 - y)*nx+x] = sse_pack(c, d, off_pbr);
            }
        }
    }
}
