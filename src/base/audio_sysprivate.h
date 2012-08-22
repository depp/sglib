#ifndef BASE_AUDIO_SYSPRIVATE_H
#define BASE_AUDIO_SYSPRIVATE_H
#include "defs.h"
#include "thread.h"
#include "audio_source.h"

/* Maximum number of mixdowns */
#define SG_AUDIO_MAXMIX 2

/* Maximum number of sources */
#define SG_AUDIO_MAXSOURCE 32

/* Maximum number of channels */
#define SG_AUDIO_MAXCHAN SG_AUDIO_MAXSOURCE

#if defined(SG_AUDIO_SYSTEM)
# define SG_INL SG_EXTERN_INLINE
#else
# define SG_INL SG_INLINE
#endif

SG_INL unsigned
sg_audio_mintime(unsigned x, unsigned y);

SG_INL unsigned
sg_audio_maxtime(unsigned x, unsigned y);

/* Earliest of the two times */
SG_INL unsigned
sg_audio_mintime(unsigned x, unsigned y)
{
    return ((int) (x - y) < 0) ? x : y;
}

/* Latest of the two times */
SG_INL unsigned
sg_audio_maxtime(unsigned x, unsigned y)
{
    return ((int) (x - y) < 0) ? y : x;
}

#undef SG_INL

/* ========== Parameter automation ========== */

/* Linear segment of audio source parameter automation.  Outside of
   the time interval, the parameter takes the value of the closest
   endpoint.  Within the time interval, the parameter is linearly
   interpolated.  */
struct sg_audio_param {
    unsigned time[2];
    float val[2];
};

/* ========== Messages ========== */

enum {
    SG_AUDIO_MSG_PLAY,
    SG_AUDIO_MSG_STOP,
    SG_AUDIO_MSG_STOPLOOP,
    SG_AUDIO_MSG_PARAM
};

#define SG_AUDIO_MSG_COUNT (SG_AUDIO_MSG_PARAM + 1)

struct sg_audio_msghdr {
    short type;
    short src;
    unsigned time;
};

struct sg_audio_msgplay {
    struct sg_audio_file *file;
    int flags;
};

struct sg_audio_msgparam {
    sg_audio_param_t param;
    unsigned endtime;
    float v1, v2;
};

/* ========== Audio system ========== */

/* Additional flags for audio sources */
enum {
    /* "Open" means that there is a handle to this sound source
       somewhere in use.  This flag is set by sg_audio_source_open()
       and reset by sg_audio_source_close().  Removing this flag will
       decrement the reference count.

       This flag is also used internally by mixdowns to indicate that
       a channel is in use.  */
    SG_AUDIO_OPEN = 1 << 16
};

struct sg_audio_source {
    /* Number of references to this sound source.  An open handle,
       marked by the SG_AUDIO_OPEN flag, counts as one reference.
       A sound currently playing also counts as a reference.  */
    unsigned refcount;
    /* If the refcount is positive, this holds the flags of current
       sound playing, as well as private flags for the audio source.
       If the refcount is zero, then this is the pointer to the next
       source in the freelist, or -1 if this is the last source in the
       freelist.  */
    unsigned flags;
    /* Current audio file playing, or NULL if no sound is playing */
    struct sg_audio_file *file;
    /* Start time of current sound playing */
    unsigned start_time;
    /* Length of current sound playing */
    unsigned length;
    /* The latest segments of parameter automation.  */
    struct sg_audio_param param[SG_AUDIO_PARAMCOUNT];
};

struct sg_audio_mixinfo {
    /* Read position of this mixdown */
    unsigned pos;
    /* Last timestamp of this mixdown */
    unsigned time;
};

struct sg_audio_system {
    struct sg_logger *log;

    /*
      System

      The system exposes its functionality through the "audio source"
      interface.  Audio source code holds the system lock, but it
      sometimes takes the queue lock as well.  Creating a new mixdown
      requires the system lock.

      Note that the system lock is sufficient for coherent read-only
      access to many of the "queue" parts.
    */

    /* Lock for interacting with the system.  */
    struct sg_lock slock;

    /* Array of audio sources.  */
    struct sg_audio_source *srcs;
    /* Count of sources that are active, open, or both */
    unsigned srccount;
    /* Size of audio source array */
    unsigned srcalloc;
    /* Index of first unused audio source */
    int srcfree;

    /* The timestamp farthest in the future.  This comes from either
       an event timestamp, the wall time, or the commit time.  */
    unsigned ftime;

    /* End+1 of last message */
    unsigned bufwpos;
    /* Start of oldest message */
    unsigned bufrpos;

    /* Mask of which mixdowns exist */
    unsigned mixmask;

    /*
      Queue

      The mixdowns interact with the system through the queue.  The
      system only needs the queue lock to commit a batch of messages,
      or to resize the queue buffer.

      Except for the mixdown fields, the queue fields can only be
      modified when holding both the system lock and the queue write
      lock.  The queue write lock should only ever be acquired when
      the system lock is already held.

      When a mixdown is running, it will update its queue position
      while holding only the queue read lock.  Creating and deleting
      mixdowns requires the system lock.
    */

    /* RW lock for interacting with the queue  */
    struct sg_rwlock qlock;

    /* Last wall time of commit */
    unsigned wtime;
    /* Last commit time */
    unsigned ctime;

    /* Ring buffer */
    void *buf;
    /* Buffer size, must be a power of 2 */
    unsigned bufsize;
    /* End+1 of last committed message */
    unsigned bufcpos;

    /* Info for each mixdown */
    struct sg_audio_mixinfo mix[SG_AUDIO_MAXMIX];
};

extern struct sg_audio_system sg_audio_system_global;

#endif
