/* Copyright 2012-2014 Dietrich Epp.
   This file is part of SGLib.  SGLib is licensed under the terms of the
   2-clause BSD license.  For more information, see LICENSE.txt. */
#include "cmdargs.h"
#include "videoproc.h"
#include "sg/error.h"
#include "sg/log.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static void
sg_videoproc_child(struct sg_cmdargs *cmd, int video_pipe)
{
    int fd, r;

    /* Note: we just count on CLOEXEC closing all file descriptors we
       don't want FFmpeg to have.  */

    if (video_pipe != 3) {
        r = dup2(video_pipe, 3);
        if (r < 0)
            abort();
    }

    fd = open("/dev/null", O_RDONLY);
    if (fd < 0)
        abort();
    if (fd != STDIN_FILENO) {
        r = dup2(fd, STDIN_FILENO);
        if (r < 0)
            abort();
        close(fd);
    }

    fd = open("/dev/null", O_WRONLY);
    if (fd < 0)
        abort();
    if (fd != STDOUT_FILENO) {
        r = dup2(fd, STDOUT_FILENO);
        if (r < 0)
            abort();
        close(fd);
    }

    execvp("ffmpeg", cmd->arg);
}

int
sg_videoproc_init(struct sg_videoproc *pp, const char *path,
                  int width, int height, struct sg_error **err)
{
    struct sg_cmdargs cmd;
    int r, fdes[2];
    pid_t pid;
    fdes[0] = -1;
    fdes[1] = -1;
    sg_cmdargs_init(&cmd);

    r = pipe(fdes);
    if (r) {
        sg_error_errno(err, errno);
        goto error;
    }

    r = sg_cmdargs_push1(&cmd, "ffmpeg");
    if (r) goto nomem;
    r = sg_cmdargs_pushf(
        &cmd,
        " -f rawvideo"
        " -pix_fmt rgb0"
        " -r 30"
        " -s %dx%d"
        " -i pipe:3"
        " -codec:v libx264"
        " -preset fast"
        " -crf 18",
        width, height);
    if (r) goto nomem;
    r = sg_cmdargs_push1(&cmd, path);
    if (r) goto nomem;

    pid = fork();
    if (pid == 0) {
        close(fdes[1]);
        sg_videoproc_child(&cmd, fdes[0]);
        abort();
    }
    if (pid < 0) {
        sg_error_errno(err, errno);
        goto error;
    }

    close(fdes[0]);
    sg_cmdargs_destroy(&cmd);

    pp->log = sg_logger_get("video");
    pp->encoder = pid;
    pp->pipe = fdes[1];
    return 0;

nomem:
    sg_error_nomem(err);
    goto error;

error:
    sg_cmdargs_destroy(&cmd);
    if (fdes[0] >= 0)
        close(fdes[0]);
    if (fdes[1] >= 0)
        close(fdes[1]);
    return -1;
}

/* Returns: 1 for running, 0 for exited.  */
static int
sg_videoproc_wait(struct sg_videoproc *pp, int hang)
{
    pid_t rpid;
    int status, retcode, flags;
    struct sg_error *err = NULL;

    flags = hang ? 0 : WNOHANG;
    rpid = waitpid(pp->encoder, &status, flags);
    if (rpid == 0) {
        return 1;
    }

    pp->encoder = -1;
    if (rpid < 0) {
        sg_error_errno(&err, errno);
        sg_logerrs(pp->log, SG_LOG_ERROR, err,
                   "could not wait for encoder");
        sg_error_clear(&err);
        pp->failed = 1;
        return 0;
    } else if (WIFEXITED(status)) {
        retcode = WEXITSTATUS(status);
        if (retcode == 0) {
            sg_logs(pp->log, SG_LOG_INFO,
                    "video encoder completed successfully");
            return 0;
        } else {
            sg_logf(pp->log, SG_LOG_ERROR,
                    "video encoder failed with return code %d",
                    retcode);
            pp->failed = 1;
            return 0;
        }
    } else if (WIFSIGNALED(status)) {
        sg_logf(pp->log, SG_LOG_ERROR,
                "video encoder received signal %d",
                WTERMSIG(status));
        pp->failed = 1;
        return 0;
    } else {
        sg_logs(pp->log, SG_LOG_ERROR,
                "unknown exit status");
        pp->failed = 1;
        return 0;
    }
}

int
sg_videoproc_destroy(struct sg_videoproc *pp, int do_kill)
{
    int r, e;
    struct sg_error *err = NULL;

    sg_videoproc_close(pp);

    if (pp->encoder > 0) {
        if (do_kill) {
            r = kill(pp->encoder, SIGTERM);
            if (r) {
                e = errno;
                if (e != ESRCH) {
                    sg_error_errno(&err, e);
                    sg_logerrs(pp->log, SG_LOG_ERROR, err,
                               "could not terminate encoder");
                    sg_error_clear(&err);
                }
            }
        }

        sg_videoproc_wait(pp, 1);
    }

    return pp->failed ? -1 : 0;
}

void
sg_videoproc_close(struct sg_videoproc *pp)
{
    int r;
    struct sg_error *err = NULL;
    if (pp->pipe < 0)
        return;
    r = close(pp->pipe);
    pp->pipe = -1;
    if (r < 0) {
        pp->failed = 1;
        sg_error_errno(&err, errno);
        sg_logerrs(pp->log, SG_LOG_ERROR, err,
                   "could not close video encoder pipe");
        sg_error_clear(&err);
    }
}

int
sg_videoproc_poll(struct sg_videoproc *pp)
{
    int r;
    if (pp->encoder <= 0) {
        return 0;
    } else {
        r = sg_videoproc_wait(pp, 0);
        return r > 0;
    }
}
