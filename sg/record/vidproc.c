/* Copyright 2012 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#define _XOPEN_SOURCE 700
/* To get dirfd with outdated libc feature tests */
#define _BSD_SOURCE 1
/* To get dirfd on Darwin */
#define _DARWIN_C_SOURCE 1

#include "sg/defs.h"
#include "sg/audio_ofile.h"
#include "sg/audio_mixdown.h"
#include "sg/dispatch.h"
#include "sg/entry.h"
#include "sg/error.h"
#include "sg/log.h"
#include "sg/record.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>

#if defined(__linux__)
# define FD_DIR "/proc/self/fd"
#elif defined(__APPLE__)
# define FD_DIR "/dev/fd"
#else
# error "Don't know how to close all fds on this platform"
#endif

#define SG_REC_MAXFRAME 16

#define SILENT_VIDENCODE 0
#define AUDIO_LOG 0

struct sg_rec_vid {
    pthread_mutex_t mutex;

    /* This variable can be set by anyone.  Locked by mutex.  */
    int stop;

    /* ==================== */

    /* Signalled by the queue writer when items are added to an empty
       queue, or when recording is stopped.  Signalled by the reader
       when the queue gets space.  */
    pthread_cond_t vid_cond;

    /* Video frame queue, locked by mutex.  */
    int vid_nframe;
    void *vid_frames[SG_REC_MAXFRAME];

    /* If stop=1, this is the timestamp of when recording stopped, or
       when recording is scheduled to stop.  Otherwise, this is the
       timestamp of the most recent video frame recorded, or if none
       have been recorded, the time when recording started.  Locked by
       mutex, only modified by the queue writer.  */
    unsigned vid_lasttime;

    /* Video information, immutable.  */

    pthread_t vid_thread;

    size_t vid_bufsz;
    int vid_width, vid_height;
    int vid_pipe;

    pid_t vid_pid;

    /* Approximate number of milliseconds per frame, rounded up.  */
    int vid_frametime;

    /* ==================== */

    /* Signalled by the queue writer when vid_lasttime passes
       aud_waittime.  */
    pthread_cond_t aud_cond;

    unsigned aud_waittime;

    /* Immutable */
    pthread_t aud_thread;

    struct sg_audio_mixdown *aud_mix;
};

static struct sg_rec_vid *sg_rec_vid;

SG_NORETURN static void
child_abort_func(const char *file, int line)
{
    char buf[256];
    size_t l, p;
    ssize_t a;
    snprintf(buf, sizeof(buf), "%s:%d: child process aborted\n", file, line);
    l = strlen(buf);
    p = 0;
    while (p < l) {
        a = write(STDERR_FILENO, buf + p, l - p);
        if (a < 0)
            break;
        p += l;
    }
    abort();
}

#define child_abort() child_abort_func(__FILE__, __LINE__)

/* Close all file descriptors >= min */
static void
sg_closefds(int min)
{
    DIR *dirp;
    int dfd;
    struct dirent *d;
    long n;
    char *e;

    dirp = opendir(FD_DIR);
    if (!dirp) abort();
    dfd = dirfd(dirp);
    while ((d = readdir(dirp))) {
        if (d->d_name[0] == '.')
            continue;
        errno = 0;
        n = strtol(d->d_name, &e, 10);
        if (errno || !e || *e)
            child_abort();
        if (n >= min && n != dfd)
            close(n);
    }
    closedir(dirp);
}

struct sg_cmd {
    char **argv;
    int argc;
    int arga;
};

static void
sg_cmd_init(struct sg_cmd *cmd)
{
    cmd->argv = NULL;
    cmd->argc = 0;
    cmd->arga = 0;
}

static void
sg_cmd_push1(struct sg_cmd *cmd, const char *p)
{
    int na;
    char **nargv;
    if (cmd->argc >= cmd->arga) {
        na = cmd->arga ? cmd->arga * 2 : 8;
        nargv = realloc(cmd->argv, sizeof(*cmd->argv) * na);
        if (!nargv)
            child_abort();
        cmd->argv = nargv;
        cmd->arga = na;
    }
    cmd->argv[cmd->argc++] = (char *) p;
}

static void
sg_cmd_pushs(struct sg_cmd *cmd, const char *str)
{
    char *p, *a, *q;
    size_t l;

    l = strlen(str);
    a = malloc(l + 1);
    if (!a)
        child_abort();
    memcpy(a, str, l + 1);

    p = a;
    while (1) {
        q = strchr(p, ' ');
        if (!q) {
            if (*p)
                sg_cmd_push1(cmd, p);
            return;
        }
        *q = '\0';
        if (p != q)
            sg_cmd_push1(cmd, p);
        p = q + 1;
    }
}

static void
sg_cmd_pushf(struct sg_cmd *cmd, const char *fmt, ...)
{
    va_list ap;
    char buf[256];
    int r;

    va_start(ap, fmt);
    r = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (r < 0 || (size_t) r >= sizeof(buf))
        child_abort();

    sg_cmd_pushs(cmd, buf);
}

/*
  FFMEG help:

  http://kylecordes.com/2007/pipe-ffmpeg (read the comments)
  http://ffmpeg.org/ffmpeg.html
*/

#include <stdio.h>

SG_NORETURN static void
sg_record_child(int vid_pipe, int width, int height)
{
    int fd, r;
    struct sg_cmd cmd;
    const char *cname = "ffmpeg";

    sg_cmd_init(&cmd);
    sg_cmd_push1(&cmd, cname);
    sg_cmd_pushf(
        &cmd,
        "-f rawvideo"
        " -pix_fmt yuv420p"
        " -r 30"
        " -s %dx%d"
        " -i pipe:3",
        width, height);
    sg_cmd_pushs(
        &cmd,
        "-vcodec mpeg2video"
        " -b 10M");
    sg_cmd_push1(&cmd, "./capture.m2v");
    sg_cmd_push1(&cmd, NULL);

    if (vid_pipe < 4 && vid_pipe != 3) {
        fd = dup(vid_pipe);
        if (fd < 0) child_abort();
        vid_pipe = fd;
    }

    /* stdin < /dev/null */
    fd = open("/dev/null", O_RDONLY);
    if (fd < 0) child_abort();
    r = dup2(fd, STDIN_FILENO);
    if (r < 0) child_abort();

    /* stdout > /dev/null */
    fd = open("/dev/null", O_WRONLY);
    if (fd < 0) child_abort();
    r = dup2(fd, STDOUT_FILENO);
    if (r < 0) child_abort();

    if (SILENT_VIDENCODE) {
        r = dup2(STDOUT_FILENO, STDERR_FILENO);
        if (r < 0) child_abort();
    }

    /* fd#3 = video pipe */
    r = dup2(vid_pipe, 3);
    if (r < 0) child_abort();

    sg_closefds(4);

    execvp(cname, cmd.argv);
    child_abort();
}

#define FAILE \
    do { line = __LINE__; goto error; } while (0)
#define FAILP \
    do { line = __LINE__; goto error_pthread; } while (0)

static void
sg_record_vidfree(void *cxt);

static void *
sg_record_vidthread(void *cxt)
{
    struct sg_rec_vid *vp;
    int r, line, ecode, fd, status;
    struct sg_error *err = NULL;
    unsigned char *buf;
    ssize_t amt;
    size_t sz, pos;
    void *retv;

    (void) line;

    vp = cxt;
    fd = vp->vid_pipe;
    sz = vp->vid_bufsz;

    while (1) {
        r = pthread_mutex_lock(&vp->mutex);
        if (r) FAILP;
        while (!vp->vid_nframe) {
            if (vp->stop)
                goto done;
            r = pthread_cond_wait(&vp->vid_cond, &vp->mutex);
            if (r) FAILP;
        }
        buf = vp->vid_frames[0];
        vp->vid_nframe--;
        if (vp->vid_nframe)
            memmove(vp->vid_frames, vp->vid_frames + 1,
                    sizeof(*vp->vid_frames) * vp->vid_nframe);
        r = pthread_mutex_unlock(&vp->mutex);
        if (r) FAILP;

        pos = 0;
        while (pos < sz) {
            amt = write(fd, buf + pos, sz - pos);
            if (amt < 0) {
                ecode = errno;
                sg_error_errno(&err, ecode);
                sg_logerrs(sg_logger_get(NULL), LOG_ERROR, err,
                           "could not write video");
                sg_error_clear(&err);
                free(buf);
                r = pthread_mutex_lock(&vp->mutex);
                if (r) FAILP;
                vp->stop = 1;
                goto done;
            }
            pos += amt;
        }
        free(buf);
    }

done:
    r = pthread_cond_signal(&vp->vid_cond);
    if (r) FAILP;
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;
    close(fd);

    r = waitpid(vp->vid_pid, &status, 0);
    if (r <= 0) {
        sg_logf(sg_logger_get(NULL), LOG_ERROR,
                   "wait returned %d", r);
    } else {
        if (WIFEXITED(status)) {
            sg_logf(sg_logger_get(NULL), LOG_INFO,
                    "video encoder returned %d", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            sg_logf(sg_logger_get(NULL), LOG_ERROR,
                    "video encoder received signal %d",
                    WTERMSIG(status));
        } else {
            sg_logf(sg_logger_get(NULL), LOG_ERROR,
                    "unknown status %d", status);
        }
    }

    /* Wait for the audio thread to finish */
    r = pthread_join(vp->aud_thread, &retv);
    if (r) FAILP;

    sg_dispatch_sync_queue(
        SG_PRE_RENDER, 20, NULL, NULL, sg_record_vidfree);

    return NULL;

error_pthread:
    abort();
}

static void *
sg_record_audiothread(void *cxt)
{
    struct sg_rec_vid *vp;
    struct sg_audio_mixdown *mix;
    struct sg_audio_ofile *afp = NULL;
    struct sg_error *err = NULL;
    int bufsz, r, i, stop, line;
    float *fbuf;
    short *sbuf;
    unsigned atime, ftime, vtime;

    (void) line;

    vp = cxt;
    mix = vp->aud_mix;
    bufsz = sg_audio_mixdown_abufsize(mix);

    fbuf = malloc(sizeof(*fbuf) * bufsz * 2);
    if (!fbuf)
        goto nomem;
    sbuf = malloc(sizeof(*sbuf) * bufsz * 2);
    if (!sbuf)
        goto nomem;

    ftime = vp->vid_frametime;

    afp = sg_audio_ofile_open(
        "capture.wav", sg_audio_mixdown_samplerate(mix), &err);
    if (!afp)
        goto error;

    while (1) {
        atime = sg_audio_mixdown_timestamp(mix) - ftime;

        r = pthread_mutex_lock(&vp->mutex);
        if (r) FAILP;
        if (AUDIO_LOG)
            printf("> %u, wait for %u\n", vp->vid_lasttime, atime);
        vp->aud_waittime = atime;
        while ((int) (vp->vid_lasttime - atime) < 0 && !vp->stop) {
            r = pthread_cond_wait(&vp->aud_cond, &vp->mutex);
            if (r) FAILP;
        }
        stop = vp->stop;
        vtime = vp->vid_lasttime;
        r = pthread_mutex_unlock(&vp->mutex);
        if (r) FAILP;

        if (stop && (int) (vtime - atime) < 0) {
            if (AUDIO_LOG)
                puts(">> stop signaled");
            break;
        }

        if (AUDIO_LOG)
            puts("> mixdown read");
        r = sg_audio_mixdown_read(mix, 0, fbuf);
        if (r) {
            sg_logs(sg_logger_get(NULL), LOG_ERROR,
                    "recording audio mixdown was reset");
            break;
        }
        if (AUDIO_LOG)
            puts("> data write");

        for (i = 0; i < bufsz; ++i) {
            sbuf[i*2+0] = (short) (32767.0f * fbuf[i*2+0]);
            sbuf[i*2+1] = (short) (32767.0f * fbuf[i*2+1]);
        }
        r = sg_audio_ofile_write(afp, sbuf, bufsz, &err);
        if (r) goto error;
    }

done:
    r = pthread_mutex_lock(&vp->mutex);
    if (r) FAILP;
    vp->stop = 1;
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;

    sg_audio_mixdown_free(mix);
    free(fbuf);
    free(sbuf);
    if (afp) {
        r = sg_audio_ofile_close(afp, &err);
        afp = NULL;
        if (r) {
            sg_logerrs(sg_logger_get(NULL), LOG_ERROR, err,
                       "error closing audio file");
            sg_error_clear(&err);
        }
    }
    if (AUDIO_LOG)
        puts(">>> AUDIO thread done");
    return NULL;

nomem:
    sg_error_nomem(&err);
    goto error;

error:
    sg_logerrs(sg_logger_get(NULL), LOG_ERROR, err,
               "could not write audio");
    sg_error_clear(&err);
    goto done;

error_pthread:
    abort();
}

static void
sg_record_spawn(int width, int height, struct sg_audio_mixdown *mix)
{
    struct sg_error *err = NULL;
    pid_t pid;
    int vid_pipe[2], r, ecode, line;
    struct sg_rec_vid *vp;
    pthread_mutexattr_t mattr;
    pthread_attr_t pattr;
    size_t bufsz;

    vid_pipe[0] = -1;
    vid_pipe[1] = -1;

    r = pipe(vid_pipe);
    if (r) FAILE;

    pid = fork();
    if (pid == 0)
        sg_record_child(vid_pipe[0], width, height);
    if (pid < 0) FAILE;

    close(vid_pipe[0]);
    vp = malloc(sizeof(*vp));
    if (!vp) FAILE;

    bufsz = width * height;
    bufsz += bufsz >> 1;

    r = pthread_mutexattr_init(&mattr);
    if (r) FAILP;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) FAILP;
    r = pthread_mutex_init(&vp->mutex, &mattr);
    if (r) FAILP;
    r = pthread_mutexattr_destroy(&mattr);
    if (r) FAILP;

    r = pthread_cond_init(&vp->vid_cond, NULL);
    if (r) FAILP;
    r = pthread_cond_init(&vp->aud_cond, NULL);
    if (r) FAILP;

    vp->vid_nframe = 0;
    vp->vid_lasttime = sg_sst.rec_ref;
    vp->vid_bufsz = bufsz;
    vp->vid_width = width;
    vp->vid_height = height;
    vp->vid_pipe = vid_pipe[1];
    vp->vid_pid = pid;
    vp->vid_frametime =
        (sg_sst.rec_denom + sg_sst.rec_numer - 1) / sg_sst.rec_numer;

    vp->aud_waittime = sg_sst.rec_ref - 10;
    vp->aud_mix = mix;

    r = pthread_attr_init(&pattr);
    if (r) FAILP;
    r = pthread_create(&vp->aud_thread, &pattr, sg_record_audiothread, vp);
    if (r) FAILP;
    r = pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
    if (r) FAILP;
    r = pthread_create(&vp->vid_thread, &pattr, sg_record_vidthread, vp);
    if (r) FAILP;
    r = pthread_attr_destroy(&pattr);
    if (r) FAILP;

    sg_rec_vid = vp;

    return;

error:
    ecode = errno;
    sg_error_errno(&err, ecode);
    sg_logerrf(sg_logger_get(NULL), LOG_ERROR, err,
               "sg_record_spawn:%d", line);
    sg_error_clear(&err);
    abort();
    return;

error_pthread:
    abort();
}

static void
sg_record_vidfree(void *cxt)
{
    struct sg_rec_vid *vp = sg_rec_vid;
    int r, i, line;

    (void) cxt;
    (void) line;

    r = pthread_mutex_destroy(&vp->mutex);
    if (r) FAILP;
    r = pthread_cond_destroy(&vp->vid_cond);
    if (r) FAILP;
    r = pthread_cond_destroy(&vp->aud_cond);
    if (r) FAILP;

    for (i = 0; i < vp->vid_nframe; ++i)
        free(vp->vid_frames[i]);

    free(vp);
    sg_rec_vid = NULL;

    sg_sst.rec_ref = 0;
    sg_sst.rec_numer = 0;
    sg_sst.rec_denom = 1000;
    sg_sst.rec_next = 0;
    sg_sst.rec_ct = 0;

    sg_record_fixsize(0, 0);

    return;

error_pthread:
    abort();
}

void
sg_record_vidstart(void)
{
    int width, height, abufsz;
    struct sg_audio_mixdown *mix;
    unsigned starttime, atime;
    struct sg_error *err = NULL;
    const char *what;

    if (sg_rec_vid)
        return;

    width = sg_sst.width;
    height = sg_sst.height;

    width = (width + 31) & ~31;
    height = (height + 1) & ~1;

    starttime = sg_sst.tick;

    abufsz = 2048;
    mix = sg_audio_mixdown_newoffline(starttime, abufsz, &err);
    if (!mix) {
        what = "could not create audio mixdown for recording";
        goto error;
    }
    atime = sg_audio_mixdown_timestamp(mix);
    if ((int) (atime - starttime) > 0)
        starttime = atime;

    sg_sst.rec_ref = starttime;
    sg_sst.rec_numer = 30;
    sg_sst.rec_denom = 1000;
    sg_sst.rec_next = starttime +
        sg_sst.rec_denom / (2 * sg_sst.rec_numer);
    sg_sst.rec_ct = 0;

    sg_record_spawn(width, height, mix);

    sg_record_fixsize(width, height);
    return;

error:
    sg_logerrs(sg_logger_get(NULL), LOG_ERROR, err, what);
    sg_error_clear(&err);
    return;
}

void
sg_record_vidstop(void)
{
    struct sg_rec_vid *vp = sg_rec_vid;
    int r, line;

    (void) line;

    if (!vp)
        return;

    r = pthread_mutex_lock(&vp->mutex);
    if (r) FAILP;
    if (!vp->stop) {
        vp->stop = 1;
        r = pthread_cond_signal(&vp->vid_cond);
        if (r) FAILP;
        r = pthread_cond_signal(&vp->vid_cond);
        if (r) FAILP;
    }
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;

    return;

error_pthread:
    abort();
}

void
sg_record_writevideo(unsigned timestamp,
                     const void *ptr, int width, int height)
{
    struct sg_rec_vid *vp = sg_rec_vid;
    int r, nf, line;
    void *buf;

    (void) line;

    if (!vp)
        return;

    assert(width == vp->vid_width && height == vp->vid_height);
    buf = malloc(vp->vid_bufsz);
    if (!buf)
        abort();

    sg_record_yuv_from_rgb(buf, ptr, width, height);

    r = pthread_mutex_lock(&vp->mutex);
    if (r) FAILP;
    while (1) {
        if (!vp->stop) {
            nf = vp->vid_nframe;
            if (nf < SG_REC_MAXFRAME) {
                vp->vid_frames[nf] = buf;
                vp->vid_nframe = nf + 1;
                if (nf == 0) {
                    r = pthread_cond_signal(&vp->vid_cond);
                    if (r) FAILP;
                }
                if ((int) (timestamp - vp->aud_waittime) >= 0) {
                    r = pthread_cond_signal(&vp->aud_cond);
                    if (r) FAILP;
                }
                vp->vid_lasttime = timestamp;
                goto done;
            }
        } else {
            free(buf);
            goto done;
        }
        r = pthread_cond_wait(&vp->vid_cond, &vp->mutex);
        if (r) FAILP;
    }
done:
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;

    return;

error_pthread:
    abort();
}
