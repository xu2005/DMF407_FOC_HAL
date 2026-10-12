#ifndef MOTION_SCHEDULER_H
#define MOTION_SCHEDULER_H
#include "motion.h"

/* Pure virtual ticks, never inferred microseconds. A synchronous sink receives
 * one axis step at its due time. Future STEP/DIR or FOC adapters must explicitly
 * define their mapping and acknowledgement; this API performs no hardware I/O.
 * Single owner; sink must not modify Motion or retain the event pointer. */
typedef int (*Motion_EventSink)(void *context, const Motion_Event *event, uint64_t ticks);
typedef enum { SCHED_ACTIVE, SCHED_COMPLETE, SCHED_CANCELLED, SCHED_FAULT, SCHED_BUDGET } Motion_SchedulerState;
typedef enum { SCHED_EVENT, SCHED_LIMIT, SCHED_DONE, SCHED_STOPPED, SCHED_ERROR, SCHED_BUSY } Motion_SchedulerResult;
typedef enum { SCHED_NO_ERROR, SCHED_SEQUENCE, SCHED_ZERO_PERIOD, SCHED_TIME_OVERFLOW, SCHED_REJECTED } Motion_SchedulerError;
typedef struct {
    uint64_t virtual_ticks, dispatched;
    Motion_Event event;
    int has_event, accepted;
    int32_t planned_x, planned_y, confirmed_x, confirmed_y;
    Motion_SchedulerState state;
    Motion_SchedulerError error;
} Motion_SchedulerSnapshot;
typedef struct {
    Motion *motion;
    Motion_EventSink sink;
    void *sink_context;
    uint64_t event_budget;
    int in_step;
    Motion_SchedulerSnapshot snapshot;
} Motion_Scheduler;
/* Start ticks and lifetime event budget are explicit. Init never requests an
 * event. Motion must already be started and have no externally pending event. */
int Motion_SchedulerInit(Motion_Scheduler *scheduler, Motion *motion,
                         Motion_EventSink sink, void *context,
                         uint64_t start_ticks, uint64_t event_budget);
Motion_SchedulerResult Motion_SchedulerStep(Motion_Scheduler *scheduler);
/* Per-call cap is resumable; lifetime budget is a latched stop. Run-to-end uses
 * remaining lifetime budget. Zero cap dispatches nothing. No sleep or malloc. */
Motion_SchedulerResult Motion_SchedulerRun(Motion_Scheduler *scheduler, uint64_t cap);
void Motion_SchedulerCancel(Motion_Scheduler *scheduler);
void Motion_SchedulerRead(const Motion_Scheduler *scheduler, Motion_SchedulerSnapshot *out);
#endif
