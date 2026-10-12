#include "v23_fixture.h"
#include <limits.h>
static Fixture f;
int main(int argc,char **argv)
{
    CHECK(argc==2); init(&f);
    if (!strcmp(argv[1],"ideal")) {
        enqueue(&f,3,-2); start(&f,0,20);
        int32_t x=0,y=0;
        while (f.motion.state==MOTION_RUNNING) {
            CHECK(Motion_SchedulerStep(&f.scheduler)==SCHED_EVENT);
            Motion_Event e=f.scheduler.snapshot.event;
            if (e.axis==MOTION_AXIS_X) { x+=e.direction; } else { y+=e.direction; }
            Motion_VirtualPlantSnapshot p=feedback(&f);
            CHECK(p.plant_x==x && p.plant_y==y && p.feedback_x==x && p.feedback_y==y);
            CHECK(p.planned_error_x==0 && p.planned_error_y==0 && p.confirmed_error_x==0 && p.confirmed_error_y==0);
        }
    } else if (!strcmp(argv[1],"reject")) {
        for (int axis=0;axis<2;++axis) {
            init(&f); enqueue(&f,axis==0 ? 10 : 0,axis==1 ? 10 : 0);
            f.plant.config.reject_sequence=3; f.plant.config.reject_axis=(Motion_Axis)axis;
            start(&f,0,20);
            CHECK(Motion_SchedulerRun(&f.scheduler,20)==SCHED_ERROR);
            CHECK(f.motion.state==MOTION_FAULT && f.scheduler.snapshot.error==SCHED_REJECTED);
            CHECK(f.plant.snapshot.received==3 && f.plant.snapshot.accepted==2 && f.plant.snapshot.rejected==1);
            CHECK(f.motion.confirmed_events==2 && f.motion.planned_events==3 && !f.motion.pending);
            Motion_VirtualPlantSnapshot p=feedback(&f);
            CHECK((axis==0 ? p.planned_error_x : p.planned_error_y)==1);
            CHECK(p.confirmed_error_x==0 && p.confirmed_error_y==0);
            CHECK(Motion_SchedulerStep(&f.scheduler)==SCHED_ERROR && f.plant.snapshot.received==3);
        }
    } else if (!strcmp(argv[1],"drop")) {
        for (int axis=0;axis<2;++axis) {
            init(&f); enqueue(&f,axis==0 ? -10 : 0,axis==1 ? -10 : 0);
            f.plant.config.drop_sequence=3; start(&f,0,20);
            CHECK(Motion_SchedulerRun(&f.scheduler,20)==SCHED_DONE);
            Motion_VirtualPlantSnapshot p=feedback(&f);
            CHECK(f.motion.confirmed_events==10 && p.accepted==10 && p.dropped==1 && p.rejected==0);
            CHECK((axis==0 ? p.confirmed_error_x : p.confirmed_error_y)==-1);
        }
    } else if (!strcmp(argv[1],"lag")) {
        Motion_VirtualPlantConfig c=Motion_VirtualPlantIdealConfig(); c.feedback_lag_events=2;
        CHECK(Motion_VirtualPlantInit(&f.plant,&c,0,0)); enqueue(&f,5,0); start(&f,0,20);
        for (int i=1;i<=5;++i) {
            CHECK(Motion_SchedulerStep(&f.scheduler)==SCHED_EVENT);
            Motion_VirtualPlantSnapshot p=feedback(&f);
            CHECK(p.plant_x==i && p.feedback_x==(i>2 ? i-2 : 0));
        }
        CHECK(feedback(&f).confirmed_error_x==2);
    } else if (!strcmp(argv[1],"encoder")) {
        Motion_VirtualPlantConfig c=Motion_VirtualPlantIdealConfig();
        c.counts_per_step[0]=4; c.counts_per_step[1]=8;
        c.quantum_counts[0]=3; c.quantum_counts[1]=5;
        c.bias_counts[0]=2; c.bias_counts[1]=-3;
        CHECK(Motion_VirtualPlantInit(&f.plant,&c,-2,3));
        CHECK(f.plant.snapshot.encoder_x==-6 && f.plant.snapshot.encoder_y==20);
        CHECK(f.plant.snapshot.feedback_x==-1.5 && f.plant.snapshot.feedback_y==2.5);
        c.feedback_lag_events=65; CHECK(!Motion_VirtualPlantInit(&f.plant,&c,0,0));
        c.feedback_lag_events=64; c.counts_per_step[0]=0; CHECK(!Motion_VirtualPlantInit(&f.plant,&c,0,0));
        c.counts_per_step[0]=INT32_MAX; c.bias_counts[0]=INT32_MAX;
        CHECK(Motion_VirtualPlantInit(&f.plant,&c,INT32_MAX,INT32_MIN));
        Motion_Event e={1,MOTION_AXIS_X,1,1,0,0};
        CHECK(!Motion_VirtualPlantSink(&f.plant,&e,1) && f.plant.snapshot.rejected==1);
        e.axis=(Motion_Axis)2; CHECK(!Motion_VirtualPlantSink(&f.plant,&e,2));
    } else { CHECK(0); }
    puts("PASS virtual axes"); return 0;
}
