#include "app_motion_sim.h"
#include <string.h>

Motion_Result App_MotionSim_Init(App_MotionSim *sim, const Motion_ProfileConfig *config)
{
    if (!sim) { return MOTION_INVALID; }
    memset(sim,0,sizeof(*sim));
    sim->motion.state=MOTION_FAULT;
    if (Motion_ProfileBuild(&sim->profile,config)!=MOTION_PROFILE_OK) { return MOTION_INVALID; }
    return Motion_Init(&sim->motion,&sim->profile,0,0);
}
Motion_Result App_MotionSim_LoadLegacyDemo(App_MotionSim *sim)
{
    /* Legacy adjustA[1][2]={{4096,0}}, Xscale=Yscale=2. */
    Motion_Result result=Motion_Enqueue(&sim->motion,4096,0,2.0,2.0);
    return result==MOTION_OK ? Motion_Start(&sim->motion) : result;
}
Motion_Result App_MotionSim_Step(App_MotionSim *sim)
{
    Motion_Event event;
    Motion_Result result=Motion_Next(&sim->motion,&event);
    if (result!=MOTION_OK) { return result; }
    result=Motion_Confirm(&sim->motion,event.sequence,1);
    if (result==MOTION_OK) { sim->last_event=event; sim->has_event=1; }
    return result;
}
void App_MotionSim_Read(const App_MotionSim *sim, App_MotionSnapshot *out)
{
    const Motion *motion=&sim->motion;
    App_MotionSnapshot snapshot={motion->state,
        motion->target_x,motion->target_y,motion->planned_x,motion->planned_y,
        motion->confirmed_x,motion->confirmed_y,motion->planned_events,motion->confirmed_events,
        motion->count,motion->profile_index,sim->last_event,sim->has_event};
    *out=snapshot;
}
