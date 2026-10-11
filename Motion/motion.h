#ifndef MOTION_H
#define MOTION_H
#include "motion_profile.h"
#include <stdint.h>

#define MOTION_DATA_LEN 100U
#define MOTION_LEGACY_INITIAL_PERIOD 10000U
typedef enum {
    MOTION_READY, MOTION_RUNNING, MOTION_COMPLETE, MOTION_CANCELLED, MOTION_FAULT
} Motion_State;
typedef enum {
    MOTION_OK, MOTION_EMPTY, MOTION_FULL, MOTION_BUSY, MOTION_INVALID, MOTION_TERMINAL
} Motion_Result;
typedef enum { MOTION_AXIS_X, MOTION_AXIS_Y } Motion_Axis;
typedef struct {
    uint64_t sequence;
    Motion_Axis axis;
    int8_t direction;
    uint16_t period;
    int32_t x, y; /* planned position after this event */
} Motion_Event;
typedef struct {
    int32_t target_x, target_y;
    uint32_t dx, dy, acclen;
    int8_t xdir, ydir;
    int decelerate;
} Motion_Segment;
typedef struct {
    const Motion_Profile *profile; /* immutable and alive until re-init */
    Motion_Segment queue[MOTION_DATA_LEN];
    uint32_t head, count, xm, ym, profile_index;
    int64_t error;
    uint16_t period;
    int32_t target_x, target_y, planned_x, planned_y, confirmed_x, confirmed_y;
    uint64_t planned_events, confirmed_events;
    Motion_Event pending_event;
    int pending;
    Motion_State state;
} Motion;

/* Single-owner context. Caller serializes enqueue/start/next/confirm/cancel.
 * No GPIO/ARR writes, waits, allocation or HAL dependencies. */
Motion_Result Motion_Init(Motion *motion, const Motion_Profile *profile, int32_t x, int32_t y);
Motion_Result Motion_Enqueue(Motion *motion, int32_t x, int32_t y, double xscale, double yscale);
Motion_Result Motion_Start(Motion *motion);
Motion_Result Motion_Next(Motion *motion, Motion_Event *event);
/* Only a matching, successfully accepted logical event changes confirmed position.
 * Rejected events latch FAULT. This is NOT a physical step acknowledgement. */
Motion_Result Motion_Confirm(Motion *motion, uint64_t sequence, int accepted);
void Motion_Cancel(Motion *motion);
void Motion_Fault(Motion *motion);
/* Explicitly open a new batch after completion/cancel. Fault requires Init.
 * Cancelling discards queued/pending events, resumes from confirmed position. */
Motion_Result Motion_Rearm(Motion *motion);
#endif
