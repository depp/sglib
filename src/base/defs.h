/* Various definitions needed for compatibility with various
   compilers (MSC) and platforms (Windows).  */
#ifndef BASE_DEFS_H
#define BASE_DEFS_H

#ifdef _MSC_VER

#if !defined(__cplusplus)
#define inline __inline
#endif

#define __attribute__(x)

#endif

#ifdef _WIN32
/* Target Windows XP */
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#endif

#endif
