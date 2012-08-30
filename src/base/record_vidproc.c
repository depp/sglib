#define _XOPEN_SOURCE 700
/* To get dirfd with outdated libc feature tests */
#define _BSD_SOURCE 1

#include "defs.h"
#include "dispatch.h"
#include "entry.h"
#include "error.h"
#include "log.h"
#include "record.h"

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

#define SG_REC_MAXFRAME 16

struct sg_rec_vid {
    /* 1.4 meg per 720p frame, 3.1 meg per 1080p frame */
    size_t bufsz;
    int width, height;
    int vid_pipe;

    pid_t pid;

    /* Everything else has a lock.  Condition variable is signaled
       when nframe rises above 0, falls below MAXFRAME, or when the
       stop variable changes.  */
    pthread_mutex_t mutex;
    pthread_cond_t cond;

    int nframe;
    void *buf[SG_REC_MAXFRAME];

    /* This variable can be set by anyone.  */
    int stop;
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

    dirp = opendir("/proc/self/fd");
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

    vp = cxt;
    fd = vp->vid_pipe;
    sz = vp->bufsz;

    while (1) {
        r = pthread_mutex_lock(&vp->mutex);
        if (r) FAILP;
        while (!vp->nframe) {
            if (vp->stop)
                goto done;
            r = pthread_cond_wait(&vp->cond, &vp->mutex);
            if (r) FAILP;
        }
        buf = vp->buf[0];
        vp->nframe--;
        if (vp->nframe)
            memmove(vp->buf, vp->buf + 1, sizeof(*vp->buf) * vp->nframe);
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
    r = pthread_cond_signal(&vp->cond);
    if (r) FAILP;
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;
    close(fd);

    r = waitpid(vp->pid, &status, 0);
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

    sg_dispatch_sync_queue(
        SG_PRE_RENDER, 20, NULL, NULL, sg_record_vidfree);

    return NULL;

error_pthread:
    abort();
}

static void
sg_record_spawn(int width, int height)
{
    struct sg_error *err = NULL;
    pid_t pid;
    int vid_pipe[2], r, ecode, line;
    struct sg_rec_vid *vp;
    pthread_mutexattr_t mattr;
    pthread_attr_t pattr;
    pthread_t thread;
    size_t bufsz;

    vid_pipe[0] = -1;
    vid_pipe[1] = -1;

    r = pipe(vid_pipe);
    if (r) FAILE;

    pid = fork();
    if (pid == 0)
        sg_record_child(vid_pipe[0], width, height);
    if (pid < 0) FAILE;

    if (0) {
        close(vid_pipe[0]);
        close(vid_pipe[1]);
        return;
    }

    close(vid_pipe[0]);
    vp = malloc(sizeof(*vp));
    if (!vp) FAILE;

    bufsz = width * height;
    bufsz += bufsz >> 1;
    vp->bufsz = bufsz;
    vp->width = width;
    vp->height = height;
    vp->vid_pipe = vid_pipe[1];
    vp->pid = pid;

    r = pthread_mutexattr_init(&mattr);
    if (r) FAILP;
    r = pthread_mutexattr_settype(&mattr, PTHREAD_MUTEX_NORMAL);
    if (r) FAILP;
    r = pthread_mutex_init(&vp->mutex, &mattr);
    if (r) FAILP;
    r = pthread_mutexattr_destroy(&mattr);
    if (r) FAILP;
    r = pthread_cond_init(&vp->cond, NULL);
    if (r) FAILP;

    vp->nframe = 0;
    vp->stop = 0;

    r = pthread_attr_init(&pattr);
    if (r) FAILP;
    r = pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);
    if (r) FAILP;
    r = pthread_create(&thread, &pattr, sg_record_vidthread, vp);
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

    r = pthread_mutex_destroy(&vp->mutex);
    if (r) FAILP;
    r = pthread_cond_destroy(&vp->cond);
    if (r) FAILP;

    for (i = 0; i < vp->nframe; ++i)
        free(vp->buf[i]);

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
    int width, height;

    if (sg_rec_vid)
        return;

    width = sg_sst.width;
    height = sg_sst.height;

    width = (width + 31) & ~31;
    height = (height + 1) & ~1;

    sg_record_spawn(width, height);
    if (0)
        return;

    sg_sst.rec_ref = sg_sst.tick;
    sg_sst.rec_numer = 30;
    sg_sst.rec_denom = 1000;
    sg_sst.rec_next = sg_sst.rec_ref +
        sg_sst.rec_denom / (2 * sg_sst.rec_numer);
    sg_sst.rec_ct = 0;

    sg_record_fixsize(width, height);
}

void
sg_record_vidstop(void)
{
    struct sg_rec_vid *vp = sg_rec_vid;
    int r, line;

    if (!vp)
        return;

    r = pthread_mutex_lock(&vp->mutex);
    if (r) FAILP;
    if (!vp->stop) {
        vp->stop = 1;
        r = pthread_cond_signal(&vp->cond);
        if (r) FAILP;
    }
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;

    return;

error_pthread:
    abort();
}

void
sg_record_writevideo(const void *ptr, int width, int height)
{
    struct sg_rec_vid *vp = sg_rec_vid;
    int r, nf, line;
    void *buf;

    if (!vp)
        return;

    assert(width == vp->width && height == vp->height);
    buf = malloc(vp->bufsz);
    if (!buf)
        abort();

    sg_record_yuv_from_rgb(buf, ptr, width, height);

    r = pthread_mutex_lock(&vp->mutex);
    if (r) FAILP;
    while (1) {
        if (!vp->stop) {
            nf = vp->nframe;
            if (nf < SG_REC_MAXFRAME) {
                vp->buf[nf] = buf;
                vp->nframe = nf + 1;
                if (nf == 0) {
                    r = pthread_cond_signal(&vp->cond);
                    if (r) FAILP;
                }
                goto done;
            }
        } else {
            free(buf);
            goto done;
        }
        r = pthread_cond_wait(&vp->cond, &vp->mutex);
        if (r) FAILP;
    }
done:
    r = pthread_mutex_unlock(&vp->mutex);
    if (r) FAILP;

    return;

error_pthread:
    abort();
}
