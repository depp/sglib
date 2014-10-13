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

/**
 * @brief An individual sprite in a sprite sheet.
 *
 * A sprite is a rectangular region in a sprite sheet, together with a
 * location marked as the origin of the rectangle.  The origin need
 * not be within the rectangle.
 */
struct sg_sprite {
    /** @brief Lower left X coordinate, from image lower left.  */
    short x;
    /** @brief Lower left Y coordinate, from image lower left.  */
    short y;
    /** @brief Width.  */
    short w;
    /** @brief Height.  */
    short h;
    /** @brief Origin X coordinate, from sprite lower left.  */
    short cx;
    /** @brief Origin Y coordinate, from sprite lower left.  */
    short cy;
};

/**
 * @brief Write a sprite to a vertex buffer.
 *
 * Writes six vertexes with four shorts each.  The six vertexes form
 * the two triangles comprising the sprite.  In each vertex, the first
 * pair is the vertex coordinates, the second pair is the texture
 * coordinates.
 *
 * @brief buffer The location where the sprite will be written.
 * @brief stride The byte offset between consecutive vertexes.
 * @brief sp The sprite texture coordinates.
 * @brief x The output X coordinate of the sprite origin.
 * @brief y The output Y coordinate of the sprite origin.
 */
void
sg_sprite_write(void *buffer, unsigned stride, struct sg_sprite sp,
                int x, int y);

/**
 * @brief Write a transformed sprite to a vertex buffer.
 *
 * Writes six vertexes with four shorts each.  The six vertexes form
 * the two triangles comprising the sprite.  In each vertex, the first
 * pair is the vertex coordinates, the second pair is the texture
 * coordinates.
 *
 * @brief buffer The location where the sprite will be written.
 * @brief stride The byte offset between consecutive vertexes.
 * @brief sp The sprite texture coordinates.
 * @brief x The output X coordinate of the sprite origin.
 * @brief y The output Y coordinate of the sprite origin.
 * @brief xform The transformation to apply to the vertex coordinates
 * before translation.
 */
void
sg_sprite_write2(void *buffer, unsigned stride, struct sg_sprite sp,
                 int x, int y, sg_sprite_xform_t xform);

#ifdef __cplusplus
}
#endif
#endif
