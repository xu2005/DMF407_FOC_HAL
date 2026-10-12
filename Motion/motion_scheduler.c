#include "motion_scheduler.h"
#include <string.h>

static void positions(Motion_Scheduler *s)
{
    s->snapshot.planned_x=s->motion->planned_x;
    s->snapshot.planned_y=s->motion->planned_y;
    s->snapshot.confirmed_x=s->motion->confirmed_x;
    s->snapshot.confirmed_y=s->motion->confirmed_y;
}
static Motion_SchedulerResult fault(Motion_Scheduler *s, Motion_SchedulerError error)
{
    Motion_Fault(s->motion);
    s->snapshot.state=SCHED_FAULT; s->snapshot.error=error;
    positions(s);
    return SCHED_ERROR;
}
static Motion_SchedulerResult terminal(Motion_Scheduler *s)
{
    positions(s);
    if (s->motion->state==MOTION_CANCELLED) { s->snapshot.state=SCHED_CANCELLED; }
    if (s->motion->state==MOTION_FAULT) { s->snapshot.state=SCHED_FAULT; }
    if (s->motion->state==MOTION_COMPLETE) { s->snapshot.state=SCHED_COMPLETE; }
    switch (s->snapshot.state) {
    case SCHED_COMPLETE: return SCHED_DONE;
    case SCHED_CANCELLED: return SCHED_STOPPED;
    case SCHED_FAULT: return SCHED_ERROR;
    case SCHED_BUDGET: return SCHED_LIMIT;
    default: return SCHED_EVENT;
    }
}
int Motion_SchedulerInit(Motion_Scheduler *s, Motion *m, Motion_EventSink sink,
                         void *context, uint64_t ticks, uint64_t budget)
{
    if (!s || !m || !sink || !budget || m->pending || m->state!=MOTION_RUNNING) { return 0; }
    memset(s,0,sizeof(*s));
    s->motion=m; s->sink=sink; s->sink_context=context; s->event_budget=budget;
    s->snapshot.virtual_ticks=ticks;
    positions(s);
    return 1;
}
Motion_SchedulerResult Motion_SchedulerStep(Motion_Scheduler *s)
{
    if (s->in_step) { return SCHED_BUSY; }
    Motion_SchedulerResult result=terminal(s);
    if (result!=SCHED_EVENT) { return result; }
    if (s->snapshot.dispatched>=s->event_budget) {
        s->snapshot.state=SCHED_BUDGET; return SCHED_LIMIT;
    }
    if (s->motion->pending || s->motion->state!=MOTION_RUNNING ||
        s->motion->planned_events!=s->motion->confirmed_events ||
        s->motion->confirmed_events==UINT64_MAX) { return fault(s,SCHED_SEQUENCE); }
    Motion_Event event;
    Motion_Result next=Motion_Next(s->motion,&event);
    if (next==MOTION_EMPTY && s->motion->state==MOTION_COMPLETE) { return terminal(s); }
    if (next!=MOTION_OK || event.sequence!=s->motion->confirmed_events+1U ||
        (event.axis!=MOTION_AXIS_X && event.axis!=MOTION_AXIS_Y) ||
        (event.direction!=1 && event.direction!=-1)) { return fault(s,SCHED_SEQUENCE); }
    if (!event.period) { return fault(s,SCHED_ZERO_PERIOD); }
    if (UINT64_MAX-s->snapshot.virtual_ticks<event.period) { return fault(s,SCHED_TIME_OVERFLOW); }
    s->snapshot.virtual_ticks+=event.period;
    s->snapshot.event=event; s->snapshot.has_event=1;
    s->in_step=1;
    int accepted=s->sink(s->sink_context,&event,s->snapshot.virtual_ticks)!=0;
    s->in_step=0;
    ++s->snapshot.dispatched; s->snapshot.accepted=accepted;
    if (s->motion->state==MOTION_CANCELLED) { return terminal(s); }
    Motion_Result confirmed=Motion_Confirm(s->motion,event.sequence,accepted);
    if (!accepted) { return fault(s,SCHED_REJECTED); }
    if (confirmed!=MOTION_OK) { return fault(s,SCHED_SEQUENCE); }
    positions(s);
    (void)terminal(s);
    return SCHED_EVENT; /* Includes the last successfully dispatched event. */
}
Motion_SchedulerResult Motion_SchedulerRun(Motion_Scheduler *s, uint64_t cap)
{
    if (s->in_step) { return SCHED_BUSY; }
    for (uint64_t i=0;i<cap;++i) {
        Motion_SchedulerResult result=Motion_SchedulerStep(s);
        if (result!=SCHED_EVENT) { return result; }
        if (s->snapshot.state!=SCHED_ACTIVE) { return terminal(s); }
    }
    Motion_SchedulerResult result=terminal(s);
    return result==SCHED_EVENT ? SCHED_LIMIT : result;
}
void Motion_SchedulerCancel(Motion_Scheduler *s)
{
    Motion_Cancel(s->motion);
    (void)terminal(s);
}
void Motion_SchedulerRead(const Motion_Scheduler *s, Motion_SchedulerSnapshot *out)
{
    *out=s->snapshot;
}
