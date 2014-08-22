/* Copyright 2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "sg/defs.h"
#include "sg/mixer.h"
#include "libpce/atomic.h"
#include "libpce/thread.h"
#include "time.h"

enum {
    /* The base two logarithm of the ratio between the audio sample
       rate and the channel parameter sample rate.  At 44.1kHz, this
       gives a parameter sample rate of 689 Hz.  */
    SG_MIXER_PARAMRATE = 6
};

typedef enum {
    SG_MIXER_LIVE,
    SG_MIXER_RECORD
} sg_mixer_which_t;

enum {
    /* Local flags.  */
    /* Control has started playback.  */
    SG_MIXER_LFLAG_START    = 1u << 0,
    /* Control has stopped playback.  */
    SG_MIXER_LFLAG_STOP     = 1u << 1,
    /* Mixers have completed playback.  */
    SG_MIXER_LFLAG_DONE     = 1u << 2,
    /* The sound was just started.  */
    SG_MIXER_LFLAG_INIT     = 1u << 3,
    /* This mixer has started playback.  */
    SG_MIXER_LFLAG_STARTED  = 1u << 4,
    /* This mixer has started stopping playback.  */
    SG_MIXER_LFLAG_STOPPED  = 1u << 5,
    /* The channel is detached.  */
    SG_MIXER_LFLAG_DETACHED = 1u << 6,

    /* Global flags, used for communication between threads.  */
    /* Control has started playback.  */
    SG_MIXER_GFLAG_START   = 1u << 0,
    /* Control has stopped playback.  */
    SG_MIXER_GFLAG_STOP    = 1u << 1,
    /* The live mixer has completed playback.  */
    SG_MIXER_GFLAG_DONE1   = 1u << 2,
    /* The recording mixer has completed playback.  */
    SG_MIXER_GFLAG_DONE2   = 1u << 3
};

struct sg_mixer_channel {
    /* Shared flags used for communication.  Locked with mutex.  */
    unsigned gflags;
    /* Local flags for use by the mixer control.  */
    unsigned lflags;
    /* The time at which playback started.  */
    unsigned starttime;
    /* The time at which playback stopped.  */
    unsigned stoptime;
    /* The sound to play.  */
    struct sg_mixer_sound *sound;
    /* Initial parameter values.  */
    float param_init[SG_MIXER_PARAM_COUNT];
    /* Current committed parameter values.  */
    float param_cur[SG_MIXER_PARAM_COUNT];
};

/* A parameter automation message.  */
struct sg_mixer_msg {
    /* The destination.  The high 16 bits give the channel, the low 16
       bits give the parameter.  */
    unsigned addr;
    /* The time at which the parameter should change.  */
    unsigned timestamp;
    /* The new parameter value.  */
    float value;
};

/* A queue of parameter automation messages.  */
struct sg_mixer_queue {
    struct sg_mixer_msg *msg;
    unsigned msgcount;
    unsigned msgalloc;
};

/* Initialize a message queue.  */
void
sg_mixer_queue_init(struct sg_mixer_queue *queue);

/* Destroy a message queue.  */
void
sg_mixer_queue_destroy(struct sg_mixer_queue *queue);

/* Allocate space for additional messages at the end of the queue, and
   return a pointer to the space allocated.  If allocation fails, log
   the error and return NULL.  */
struct sg_mixer_msg *
sg_mixer_queue_append(struct sg_mixer_queue *SG_RESTRICT queue,
                      unsigned count);

/* Per-mixdown channel state.  */
struct sg_mixer_mixchan {
    /* Local channel flags.  */
    unsigned flags;
    /* The last value of each parameter.  */
    float param[SG_MIXER_PARAM_COUNT];
    /* The current sample position.  */
    unsigned samplepos;
};

/* A mixdown.  There may be a separate mixdown for live audio and
   recording audio to disk.  */
struct sg_mixer_mixdown {
    /* Message queue for messages being processed.  */
    struct sg_mixer_queue procqueue;

    /* The identity of this mixer: 0 = live, 1 = recording.  */
    sg_mixer_which_t which;

    /* The local state of the mixer channels.  */
    struct sg_mixer_mixchan *SG_RESTRICT channel;
    unsigned channelcount;

    /* The size of each audio buffer.  Parameter buffers are scaled
       down to use fewer samples.  */
    int bufsz;

    /* Audio buffers, each bufsz samples long.  The first two are
       input buffers for the channel being processed.  The remaining
       buffers are bus outputs.  */
    float *SG_RESTRICT audio_buf;

    /* Parameter buffers, each bufsz >> PARAMRATE samples long.  */
    float *SG_RESTRICT param_buf;

    /* Translation between timestamps and sample positions.  */
    union {
        struct sg_mixer_time delayed;
        struct sg_mixer_timeexact exact;
    } time;
};

/* Interface to a mixdown.  Contains structures accessed from outside
   the mixdown code itself.  This way, the sg_mixer_mixdown can be
   restrict qualified when we want.  */
struct sg_mixer_mixdowniface {
    /* Incoming message queue, which can only be read or modified when
       the global sg_mixer lock is held.  */
    struct sg_mixer_queue inqueue;

    /* The mixdown, which can only be modified by the thread which is
       processing the mixdown.  */
    struct sg_mixer_mixdown mixdown;
};

/* Create a new live mixdown.  Events will immediately be pushed to
   the mixdown.  When the audio system runs from sg_mixer_start(), it
   should first call create_live(), then repeatedly call process(),
   and eventually call destroy().  */
struct sg_mixer_mixdowniface *
sg_mixer_mixdown_create_live(int bufsz, int samplerate,
                             struct sg_error **err);

/* Create a new recording mixdown.  The system will automatically
   process the recording mixdown.  */
int
sg_mixer_mixdown_create_record(unsigned starttime,
                               struct sg_error **err);

/* Destroy a mixdown.  */
void
sg_mixer_mixdown_destroy(struct sg_mixer_mixdowniface *mp);

/* Process mixer events and render audio.  The buffertime parameter
   gives the timestamp of the end of the rendered buffer for live
   audio, for recorded audio it is ignored.  Returns the number of
   samples rendered, or 0 if the mixdown has halted.  */
int
sg_mixer_mixdown_process(struct sg_mixer_mixdowniface *mp,
                         unsigned buffertime);

/* Get the mixer output, as interleaved 32-bit floating-point
   samples.  */
void
sg_mixer_mixdown_get_f32(struct sg_mixer_mixdowniface *mp,
                         float *buffer);

/* Global mixer system state.  */
struct sg_mixer {
    /* Global lock used for communication between different threads
       that use the mixer.  Contention is kept to a minimum: this is
       only locked when committing mixer commands, when receiving
       mixer commands, and when creating or destroying mixers.  */
    struct pce_lock lock;

    /* Indicates that sg_mixer_commit() has been called at least
       once.  */
    int is_ready;

    /* The current time, not committed yet.  */
    unsigned time;

    /* The commit timestamp.  All future messages should have later
       timestamps than the commit timestamp.  */
    unsigned committime;

    /* The control structures for each of the channels, used to issue
       commands to the mixdowns.  */
    struct sg_mixer_channel *channel;
    unsigned channelcount;

    /* The queue of uncommitted parameter messages.  */
    struct sg_mixer_queue queue;

    /* The mixdowns: the live mixdown renders to the audio output, the
       recording mixdown renders to a file.  */
    struct sg_mixer_mixdowniface *mix_live, *mix_record;
};

/* Initialize the mixer system output.  */
void
sg_mixer_system_init(void);

/* Global mixer instance.  */
extern struct sg_mixer sg_mixer;
