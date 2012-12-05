/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_SYSTEM_H
#define SG_AUDIO_SYSTEM_H
#ifdef __cplusplus
extern "C" {
#endif

/* Initialize the main audio system.  */
void
sg_audio_sys_init(void);

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
