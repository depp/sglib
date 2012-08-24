#include "audio_system.h"
#include "audio_sysprivate.h"
#include "log.h"

struct sg_audio_system sg_audio_system_global;

void
sg_audio_sys_init(void)
{
    struct sg_audio_system *sp = &sg_audio_system_global;
    sp->log = sg_logger_get("audio");
    sg_lock_init(&sp->slock);
    sg_rwlock_init(&sp->qlock);
    sp->srcfree = -1;
}
