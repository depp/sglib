#ifndef BASE_DISPATCH_IMPL_H
#define BASE_DISPATCH_IMPL_H
#ifdef __cplusplus
extern "C" {
#endif

/* This should be treated as an opaque structure by everything but
   dispatch_common.c.  */
struct sg_dispatch_task {
    /* Task priority.  The high 16 bits are the user assigned
       priority, and the low 16 bits are a counter.  Due to rollover,
       an older task will be prioritized lower if enough new tasks
       have been added since the older tasks.  The number of new tasks
       required is at least 2^15, so if rollover happens the old
       thread probably won't execute anyway.  */
    unsigned priority;

    /* Callback.  */
    void *cxt;
    void (*func)(void *);
};

struct sg_dispatch_queue {
    /* The number of tasks in this queue.  */
    unsigned count;

    /* The base two logarithm of one plus the size of the array
       holding queued tasks.  This is also the depth of the heap.  */
    unsigned depth;

    /* Counter for tasks inserted, so tasks inserted later get
       prioritized lower relative to tasks at the same assigned
       priority level.  */
    unsigned ctr;

    /* Binary max heap for task priority queue.  */
    struct sg_dispatch_task *tasks;
};

/* Add a task to the queue.  */
void
sg_dispatch_queue_push(struct sg_dispatch_queue *queue,
                         int priority, void *cxt, void (*func)(void *));

/* Pop the highest priority task from the queue.  This will abort the
   program if the queue is empty.  */
void
sg_dispatch_queue_pop(struct sg_dispatch_queue *queue,
                      struct sg_dispatch_task *task);

/* Execute the given task.  */
void
sg_dispatch_task_exec(struct sg_dispatch_task *task);

#ifdef __cplusplus
}
#endif
#endif
