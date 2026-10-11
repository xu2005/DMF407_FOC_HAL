#ifndef APP_MOTION_SIM_H
#define APP_MOTION_SIM_H
#include "motion.h"
typedef struct {
    Motion_Profile profile;
    Motion motion;
    Motion_Event last_event;
    int has_event;
} App_MotionSim;
typedef struct {
    Motion_State state;
    int32_t target_x, target_y, planned_x, planned_y, confirmed_x, confirmed_y;
    uint64_t planned_events, confirmed_events;
    uint32_t queued_segments, profile_index;
    Motion_Event last_event;
    int has_event;
} App_MotionSnapshot;

Motion_Result App_MotionSim_Init(App_MotionSim *sim, const Motion_ProfileConfig *config);
Motion_Result App_MotionSim_LoadLegacyDemo(App_MotionSim *sim);
/* One software event per call, immediately accepted by the logical sink.
 * period is trace data, not elapsed time; no physical pulse/position feedback. */
Motion_Result App_MotionSim_Step(App_MotionSim *sim);
void App_MotionSim_Read(const App_MotionSim *sim, App_MotionSnapshot *out);
#endif
