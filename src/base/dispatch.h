#ifndef BASE_DISPATCH_H
#define BASE_DISPATCH_H
#ifdef __cplusplus
extern "C" {
#endif

/* The task type.  Selecting the correct type allows the dispatcher to
   optimize usage of available computational power and IO
   bandwidth.  */
typedef enum {
    /* A task limited by CPU or memory.  */
    SG_DISPATCH_NORMAL,

    /* A task limited by disk IO.  */
    SG_DISPATCH_IO
} sg_dispatch_type_t;

/* Priority limits.  Tasks with higher priority (larger numbers)
   execute before tasks with lower priority (lower numbers).  */
enum {
    SG_DISPATCH_PRIO_MAX = 32767,
    SG_DISPATCH_PRIO_MIN = -32768
};

/* Add a task to the queue to be completed asynchronously.  */
void
sg_dispatch_queue(sg_dispatch_type_t type, int priority,
                  void *cxt, void (*func)(void *));

/* Change the task type of the currently executing task.  */
void
sg_dispatch_settype(sg_dispatch_type_t type);

#ifdef __cplusplus
}
#endif
#endif
