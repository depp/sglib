/* Copyright 2012 Dietrich Epp <depp@zdome.net> */
#ifndef SG_AUDIO_SOURCE_H
#define SG_AUDIO_SOURCE_H
#ifdef __cplusplus
extern "C" {
#endif
struct sg_audio_file;

/*
  Farthest time in the future permitted for any audio system event,
  measured in ticks from the previous wall time.  Any event which is
  scheduled farther in the future will get moved to this time.  About
  four seconds.

  This is short to limit the delay when starting or restarting audio
  mixdowns.  Due to the internal design of the audio system, a new
  audio mixdown must wait until all future events have passed until it
  can start producing audio.  (The alternative would require
  maintaining an extra copy of the entire audio system state.)
*/
#define SG_AUDIO_MAXTIME (1 << 12)

/*
  Maximum length of time for a parameter transition, in ticks.  About
  four and a half minutes.  Any parameter automation event longer than
  this will be shortened to fit.  Note that this is far longer than
  the farthest ahead an event can be scheduled -- this is because
  parameter automation events effectively "occur" on the moment they
  start, and the moment they end is less important for scheduling.
*/
#define SG_AUDIO_MAXPTIME (1 << 18)

/*
  On error handling:

  The audio functions communicate asynchronously with audio servers in
  other threads.  Errors are mostly ignored, or logged.
*/

/* Note: these flags don't do anything yet... */
enum {
    /* Always play this sound, even if it has to be delayed in order
       for the file to load.  */
    SG_AUDIO_NODROP = 1 << 0,
    /* Exclude this sound from latency adjustments.  This can be used
       for UI sounds.  */
    SG_AUDIO_NOLATENCY = 1 << 1,
    /* Continuously loop the sound.  */
    SG_AUDIO_LOOP = 1 << 2
};

/* The quietest sound level */
#define SG_AUDIO_SILENT (-80.0f)

/* Audio parameters.  */
typedef enum {
    /* Volume, measured in dB.  0 dB is full volume.  Sounds below -60
       dB are attenuated more sharply so -80 dB is actually mapped to
       silence.  */
    SG_AUDIO_VOL,
    /* Pan, from -1 (full left) to +1 (full right) */
    SG_AUDIO_PAN
} sg_audio_param_t;


#define SG_AUDIO_PARAMCOUNT ((int) SG_AUDIO_PAN + 1)

/* Return a reference to a new sound source, or return -1 if it could
   not be allocated.  An error will also be logged on failure.  */
int
sg_audio_source_open(void);

/* Close a reference to a sound source.  The sound will continue to
   play until it ends.  The sound will have looping disabled.  */
void
sg_audio_source_close(int src);

/* Play the given audio file.  */
void
sg_audio_source_play(int src, unsigned time, struct sg_audio_file *file,
                     int flags);

/* Stop playing the audio source.  */
void
sg_audio_source_stop(int src, unsigned time);

/* Stop looping the audio source.  The current loop will finish.  */
void
sg_audio_source_stoploop(int src, unsigned time);

/* Adjust an audio parameter, with a sudden step to the target
   value */
void
sg_audio_source_pset(int src, sg_audio_param_t param,
                     unsigned time, float value);

/* Adjust an audio parameter, moving it linearly to the target value
   over the given time interval.  */
void
sg_audio_source_plinear(int src, sg_audio_param_t param,
                        unsigned time_start, unsigned time_end,
                        float value);

/* Adjust an audio parameter, moving it linearly tothe target value at
   the given slope, starting at the given time.  */
void
sg_audio_source_pslope(int src, sg_audio_param_t param,
                       unsigned time, float value, float slope);


/* Commit to not producing any sounds or audio events with timestamps
   before the given timestamps.  Note: if this function is
   not called,
   audio events will not be processed, i.e., you will get no sound and
   the buffers will fill up.  The wall time (wtime) is the current
   time, and no future NOLATENCY sounds should be generated before it.
   The commit time (ctime) is the time for all other sounds.  */
void
sg_audio_source_commit(unsigned wtime, unsigned ctime);

#ifdef __cplusplus
}
#endif
#endif
