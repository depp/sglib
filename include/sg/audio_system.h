/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#ifndef SG_AUDIO_SYSTEM_H
#define SG_AUDIO_SYSTEM_H
#ifdef __cplusplus
extern "C" {
#endif

/* Start the platform's audio system.  */
void
sg_audio_sys_pstart(void);

/* Stop the platform's audio system.  */
void
sg_audio_sys_pstop(void);

#ifdef __cplusplus
}
#endif
#endif
