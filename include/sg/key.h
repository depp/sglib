/* Copyright 2011-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_KEY_H
#define SG_KEY_H
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file key.h
 *
 * @brief Keyboard code utilities.
 */

/**
 * @brief Convert a key ID to an HID keycode.
 *
 * @param name The key ID.
 * @return The HID keycode, or -1 if no keycode has the given name.
 */
int
sg_keyid_code_from_name(const char *name);

/**
 * @brief Convert an HID keycode to a key ID.
 *
 * @param code The HID keycode.
 * @return The key ID, or `NULL` if the HID keycode is invalid or has
 * no name.
 */
const char *
sg_keyid_name_from_code(int code);

/**
 * @brief Table mapping HID keycodes to Mac keycodes.
 */
extern const unsigned char SG_MAC_HID_TO_NATIVE[256];

/**
 * @brief Table mapping Mac keycodes to HID keycodes.
 */
extern const unsigned char SG_MAC_NATIVE_TO_HID[128];

/**
 * @brief Table mapping HID keycodes to Evdev (Linux) keycodes.
 */
extern const unsigned char SG_EVDEV_HID_TO_NATIVE[256];

/**
 * @brief Table maping Evdev (Linux) keycodes to HID keycodes.
 */
extern const unsigned char SG_EVDEV_NATIVE_TO_HID[256];

/**
 * @brief Table mapping HID keycodes to Windows keycodes.
 */
extern const unsigned char SG_WIN_HID_TO_NATIVE[256];

/**
 * @brief Table mapping Windows keycodes to HID keycodes.
 */
extern const unsigned char SG_WIN_NATIVE_TO_HID[256];

#ifdef __cplusplus
}
#endif
#endif
