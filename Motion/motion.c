#include "motion.h"
#include <math.h>
#include <limits.h>
#include <string.h>

static uint32_t major(const Motion_Segment *segment)
{
    return segment->dx >= segment->dy ? segment->dx : segment->dy;
}

Motion_Result Motion_Init(Motion *motion, const Motion_Profile *profile, int32_t x, int32_t y)
{
    if (!motion) { return MOTION_INVALID; }
    memset(motion,0,sizeof(*motion));
    motion->state=MOTION_FAULT;
    if (!profile || !profile->valid || profile->dec_len==0 || profile->dec_len>MOTION_ARRAY_LENGTH) {
        return MOTION_INVALID;
    }
    motion->profile=profile;
    motion->target_x=motion->planned_x=motion->confirmed_x=x;
    motion->target_y=motion->planned_y=motion->confirmed_y=y;
    /* Time4_Init precedes cul_Saccdec in Legacy main: initial ARR is 10000,
     * including a first minor-X event on a Y-dominant segment. */
    motion->period=MOTION_LEGACY_INITIAL_PERIOD;
    motion->state=MOTION_READY;
    return MOTION_OK;
}

Motion_Result Motion_Enqueue(Motion *motion, int32_t x, int32_t y, double xscale, double yscale)
{
    if (!motion || motion->state!=MOTION_READY) { return MOTION_BUSY; }
    if (motion->count==MOTION_DATA_LEN) { return MOTION_FULL; }
    if (!isfinite(xscale) || !isfinite(yscale)) { return MOTION_INVALID; }
    double scaled_x=(double)x*xscale, scaled_y=(double)y*yscale;
    if (!isfinite(scaled_x) || !isfinite(scaled_y) ||
        scaled_x<INT32_MIN || scaled_x>INT32_MAX || scaled_y<INT32_MIN || scaled_y>INT32_MAX) {
        return MOTION_INVALID;
    }
    int32_t tx=(int32_t)scaled_x, ty=(int32_t)scaled_y; /* Legacy truncation */
    int64_t dx=(int64_t)tx-motion->target_x, dy=(int64_t)ty-motion->target_y;
    uint32_t tail=(motion->head+motion->count)%MOTION_DATA_LEN;
    Motion_Segment *segment=&motion->queue[tail];
    segment->target_x=tx; segment->target_y=ty;
    segment->xdir=dx<0 ? -1 : 1; segment->ydir=dy<0 ? -1 : 1;
    segment->dx=(uint32_t)(dx<0 ? -dx : dx);
    segment->dy=(uint32_t)(dy<0 ? -dy : dy);
    segment->acclen=0; segment->decelerate=0;
    motion->target_x=tx; motion->target_y=ty;
    ++motion->count;
    return MOTION_OK;
}

Motion_Result Motion_Start(Motion *motion)
{
    if (!motion || motion->state!=MOTION_READY) { return MOTION_BUSY; }
    if (!motion->count) { return MOTION_EMPTY; }
    /* speed_plane: consider exactly the populated batch, never the empty tail.
     * Original distance is max(abs(dx),abs(dy)), despite one-axis events. */
    uint64_t total=0, suffix=0;
    for (uint32_t i=0; i<motion->count; ++i) {
        total+=major(&motion->queue[(motion->head+i)%MOTION_DATA_LEN]);
    }
    uint64_t span=total >= 2U*motion->profile->dec_len ? motion->profile->dec_len : total/2U+1U;
    for (uint32_t i=motion->count; i>0; --i) {
        Motion_Segment *segment=&motion->queue[(motion->head+i-1)%MOTION_DATA_LEN];
        suffix+=major(segment);
        segment->decelerate=1;
        segment->acclen=0;
        if (suffix>=span) { segment->acclen=(uint32_t)(suffix-span); break; }
    }
    motion->xm=motion->ym=0; motion->error=0;
    motion->state=MOTION_RUNNING;
    return MOTION_OK;
}

static void retire_segment(Motion *motion)
{
    motion->head=(motion->head+1U)%MOTION_DATA_LEN;
    --motion->count;
    motion->xm=motion->ym=0; motion->error=0;
    if (!motion->count) { motion->state=MOTION_COMPLETE; }
}

Motion_Result Motion_Next(Motion *motion, Motion_Event *event)
{
    if (!motion || !event) { return MOTION_INVALID; }
    if (motion->state!=MOTION_RUNNING) { return MOTION_TERMINAL; }
    if (motion->pending) { return MOTION_BUSY; }
    /* At most DataLen zero-length segments; no unbounded loops/waits. */
    while (motion->count && major(&motion->queue[motion->head])==0) { retire_segment(motion); }
    if (!motion->count) { return MOTION_EMPTY; }
    Motion_Segment *segment=&motion->queue[motion->head];
    Motion_Axis axis=(motion->error>=0 && motion->xm<segment->dx) ? MOTION_AXIS_X : MOTION_AXIS_Y;
    uint32_t progress=axis==MOTION_AXIS_X ? motion->xm : motion->ym;
    int is_major=axis==MOTION_AXIS_X ? segment->dx>=segment->dy : segment->dy>segment->dx;
    if (is_major) {
        if (segment->decelerate && progress>=segment->acclen) {
            if (motion->profile_index>1U) { --motion->profile_index; }
        } else {
            uint16_t current=motion->profile->period[motion->profile_index];
            if (current>motion->profile->period_min && motion->profile_index<motion->profile->dec_len) {
                ++motion->profile_index;
            } else if (current<motion->profile->period_min && motion->profile_index>0) {
                --motion->profile_index;
            }
        }
        motion->period=motion->profile->period[motion->profile_index];
    }
    if (axis==MOTION_AXIS_X) {
        ++motion->xm; motion->error-=segment->dy;
        motion->planned_x+=segment->xdir;
    } else {
        ++motion->ym; motion->error+=segment->dx;
        motion->planned_y+=segment->ydir;
    }
    Motion_Event next={++motion->planned_events,axis,
        axis==MOTION_AXIS_X ? segment->xdir : segment->ydir,
        motion->period,motion->planned_x,motion->planned_y};
    motion->pending_event=next; motion->pending=1;
    *event=next;
    return MOTION_OK;
}

Motion_Result Motion_Confirm(Motion *motion, uint64_t sequence, int accepted)
{
    if (!motion || motion->state!=MOTION_RUNNING || !motion->pending ||
        sequence!=motion->pending_event.sequence) { return MOTION_INVALID; }
    if (!accepted) { Motion_Fault(motion); return MOTION_TERMINAL; }
    motion->confirmed_x=motion->pending_event.x;
    motion->confirmed_y=motion->pending_event.y;
    ++motion->confirmed_events;
    motion->pending=0;
    Motion_Segment *segment=&motion->queue[motion->head];
    if (motion->xm==segment->dx && motion->ym==segment->dy) { retire_segment(motion); }
    return MOTION_OK;
}

static void stop(Motion *motion, Motion_State state)
{
    if (!motion) { return; }
    motion->head=(motion->head+motion->count)%MOTION_DATA_LEN;
    motion->count=0; motion->pending=0; motion->state=state;
}
void Motion_Cancel(Motion *motion)
{
    if (motion && motion->state!=MOTION_FAULT) { stop(motion,MOTION_CANCELLED); }
}
void Motion_Fault(Motion *motion) { stop(motion,MOTION_FAULT); }

Motion_Result Motion_Rearm(Motion *motion)
{
    if (!motion || (motion->state!=MOTION_COMPLETE && motion->state!=MOTION_CANCELLED)) { return MOTION_BUSY; }
    motion->target_x=motion->planned_x=motion->confirmed_x;
    motion->target_y=motion->planned_y=motion->confirmed_y;
    motion->profile_index=0; motion->period=MOTION_LEGACY_INITIAL_PERIOD;
    motion->xm=motion->ym=0; motion->error=0;
    motion->state=MOTION_READY;
    return MOTION_OK;
}
