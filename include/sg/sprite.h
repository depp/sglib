/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_SPRITE_H
#define SG_SPRITE_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sg/sprite.h
 *
 * @brief Sprite sheets.
 */

/**
 * @brief Sprite transformations.
 *
 * These represent any series of 90 degree rotations and mirroring
 * about an axis.  The enumeration order should not be modified, the
 * low two bits count the number of 90 degree rotations and the high
 * bit indicates that rotation is followed by vertical mirroring.
 */
typedef enum {
    /** @brief No transformation.  */
    SG_X_NORMAL = 0,
    /** @brief 90 degree counterclockwise rotation.  */
    SG_X_ROTATE_90 = 1,
    /** @brief 180 degree rotation.  */
    SG_X_ROTATE_180 = 2,
    /** @brief 270 degree counterclockwise rotation..  */
    SG_X_ROTATE_270 = 3,
    /** @brief Vertical flip.  */
    SG_X_FLIP_VERTICAL = 4,
    /** @brief Flip about the line y = -x.  */
    SG_X_TRANSPOSE_2 = 5,
    /** @brief Horizontal flip.  */
    SG_X_FLIP_HORIZONTAL = 6,
    /** @brief Flip about the line y = x.  */
    SG_X_TRANSPOSE = 7
} sg_sprite_xform_t;

/**
 * @brief Compose two sprite transformations.
 *
 * @param x The outer transformation.
 * @param y The inner transformation.
 * @return A transformation equivalent to applying @c y first and then
 * @c x.
 */
sg_sprite_xform_t
sg_sprite_xform_compose(sg_sprite_xform_t x, sg_sprite_xform_t y);

#ifdef __cplusplus
}
#endif
#endif
