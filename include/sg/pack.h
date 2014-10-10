/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_PACK_H
#define SG_PACK_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_error;

/**
 * @file sg/pack.h
 *
 * @brief Rectangle packing.
 */

/**
 * @brief A rectangle with integer coordinates.
 */
struct sg_pack_rect {
    /** @brief The rectangle width.  */
    unsigned short w;
    /** @brief The rectangle height.  */
    unsigned short h;
    /** @brief The X coordinate of the rectangle lower left.  */
    unsigned short x;
    /** @brief The Y coordinate of the rectangle lower left.  */
    unsigned short y;
};

/**
 * @brief A size with integer coordinates.
 */
struct sg_pack_size {
    /** @brief Width.  */
    unsigned short width;
    /** @brief Height.  */
    unsigned short height;
};

/**
 * @brief Pack a set of rectangles in a larger rectangle.
 *
 * @param rect The array of rectangles, with width and height
 * specified.  If this function is successful, it will set the
 * rectangle locations.
 *
 * @param rectcount The number of rectangles in the array.
 *
 * @param size On success, the size of the packing.  The dimensions
 * will be powers of two.
 *
 * @param maxsize The maximum size of the packing.
 *
 * @param err On failure, the error.
 *
 * @return If successful, a positive number is returned.  If a packing
 * could not be found that fits the given constraints, zero is
 * returned.  If an error occurs, a negative number is returned.
 */
int
sg_pack(struct sg_pack_rect *rect, unsigned rectcount,
        struct sg_pack_size *size, const struct sg_pack_size *maxsize,
        struct sg_error **err);

#ifdef __cplusplus
}
#endif
#endif
