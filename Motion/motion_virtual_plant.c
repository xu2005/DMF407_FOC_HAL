#include "motion_virtual_plant.h"
#include <limits.h>
#include <string.h>

Motion_VirtualPlantConfig Motion_VirtualPlantIdealConfig(void)
{
    Motion_VirtualPlantConfig config={{1,1},{1,1},{0,0},0,0,0,MOTION_AXIS_X};
    return config;
}
static void sample(Motion_VirtualPlant *p, int32_t x, int32_t y)
{
    int32_t steps[2]={x,y};
    int64_t counts[2];
    double feedback[2];
    for (unsigned a=0;a<2;++a) {
        counts[a]=(int64_t)steps[a]*p->config.counts_per_step[a]+p->config.bias_counts[a];
        counts[a]=(counts[a]/p->config.quantum_counts[a])*p->config.quantum_counts[a];
        feedback[a]=(double)counts[a]/p->config.counts_per_step[a];
    }
    p->snapshot.encoder_x=counts[0]; p->snapshot.encoder_y=counts[1];
    p->snapshot.feedback_x=feedback[0]; p->snapshot.feedback_y=feedback[1];
}
int Motion_VirtualPlantInit(Motion_VirtualPlant *p, const Motion_VirtualPlantConfig *c,
                           int32_t x, int32_t y)
{
    if (!p || !c || c->feedback_lag_events>MOTION_FEEDBACK_MAX_LAG ||
        (c->reject_axis!=MOTION_AXIS_X && c->reject_axis!=MOTION_AXIS_Y)) { return 0; }
    for (unsigned a=0;a<2;++a) {
        if (!c->counts_per_step[a] || c->counts_per_step[a]>INT32_MAX ||
            !c->quantum_counts[a] || c->quantum_counts[a]>INT32_MAX) { return 0; }
    }
    memset(p,0,sizeof(*p)); p->config=*c;
    p->snapshot.plant_x=x; p->snapshot.plant_y=y;
    for (unsigned i=0;i<=MOTION_FEEDBACK_MAX_LAG;++i) { p->history[i][0]=x; p->history[i][1]=y; }
    sample(p,x,y);
    return 1;
}
int Motion_VirtualPlantSink(void *context, const Motion_Event *e, uint64_t ticks)
{
    Motion_VirtualPlant *p=context;
    ++p->snapshot.received; p->snapshot.virtual_ticks=ticks;
    if ((e->axis!=MOTION_AXIS_X && e->axis!=MOTION_AXIS_Y) ||
        (e->direction!=1 && e->direction!=-1) ||
        (e->sequence==p->config.reject_sequence && e->axis==p->config.reject_axis)) {
        ++p->snapshot.rejected; return 0;
    }
    int32_t *position=e->axis==MOTION_AXIS_X ? &p->snapshot.plant_x : &p->snapshot.plant_y;
    if (e->sequence==p->config.drop_sequence) { ++p->snapshot.dropped; }
    else {
        if ((e->direction>0 && *position==INT32_MAX) || (e->direction<0 && *position==INT32_MIN)) {
            ++p->snapshot.rejected; return 0;
        }
        *position+=e->direction;
    }
    ++p->snapshot.accepted;
    uint32_t length=p->config.feedback_lag_events+1U;
    p->history[p->head][0]=p->snapshot.plant_x;
    p->history[p->head][1]=p->snapshot.plant_y;
    p->head=(p->head+1U)%length;
    sample(p,p->history[p->head][0],p->history[p->head][1]);
    return 1;
}
void Motion_VirtualPlantRead(const Motion_VirtualPlant *p, const Motion_SchedulerSnapshot *s,
                            Motion_VirtualPlantSnapshot *out)
{
    *out=p->snapshot;
    out->planned_error_x=(double)s->planned_x-out->feedback_x;
    out->planned_error_y=(double)s->planned_y-out->feedback_y;
    out->confirmed_error_x=(double)s->confirmed_x-out->feedback_x;
    out->confirmed_error_y=(double)s->confirmed_y-out->feedback_y;
}
