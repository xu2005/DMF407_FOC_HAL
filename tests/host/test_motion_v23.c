#include "v23_fixture.h"
static Fixture f;
static void finish(int32_t x,int32_t y,uint64_t events)
{
    uint64_t before=f.motion.confirmed_events;
    start(&f,0,20000);
    CHECK(Motion_SchedulerRun(&f.scheduler,20000)==SCHED_DONE);
    Motion_VirtualPlantSnapshot p=feedback(&f);
    CHECK(p.plant_x==x && p.plant_y==y && p.feedback_x==x && p.feedback_y==y);
    CHECK(f.scheduler.snapshot.dispatched==events && f.motion.confirmed_events==before+events);
    CHECK(p.confirmed_error_x==0 && p.confirmed_error_y==0);
}
int main(int argc,char **argv)
{
    CHECK(argc==2);
    if (!strcmp(argv[1],"geometry")) {
        const int32_t targets[][2]={{20,0},{0,20},{20,20},{23,7},{7,23},{-23,7},{-23,-7},{23,-7}};
        for (unsigned i=0;i<sizeof(targets)/sizeof(targets[0]);++i) {
            init(&f); int32_t tx=targets[i][0],ty=targets[i][1]; enqueue(&f,tx,ty); start(&f,0,100);
            uint64_t count=0;
            while (f.motion.state==MOTION_RUNNING) {
                CHECK(Motion_SchedulerStep(&f.scheduler)==SCHED_EVENT);
                Motion_VirtualPlantSnapshot p=feedback(&f);
                /* Ordered axis steps stay within one lattice-cell line error.
                 * This is geometric coordination, not simultaneous pulses. */
                CHECK(llabs((long long)p.plant_x*ty-(long long)p.plant_y*tx)<=abs(tx)+abs(ty));
                ++count;
            }
            CHECK(count==(uint64_t)(abs(tx)+abs(ty)) && f.plant.snapshot.plant_x==tx && f.plant.snapshot.plant_y==ty);
        }
    } else if (!strcmp(argv[1],"segments")) {
        init(&f); enqueue(&f,0,0); enqueue(&f,10,5); enqueue(&f,10,5);
        enqueue(&f,-3,8); enqueue(&f,0,0); enqueue(&f,0,0); finish(0,0,42);
        init(&f); enqueue(&f,0,0); finish(0,0,0);
    } else if (!strcmp(argv[1],"queue")) {
        init(&f);
        for (int i=1;i<=100;++i) { enqueue(&f,i,0); }
        CHECK(Motion_Enqueue(&f.motion,101,0,1,1)==MOTION_FULL); finish(100,0,100);
        CHECK(Motion_Rearm(&f.motion)==MOTION_OK);
        for (int i=99;i>=0;--i) { enqueue(&f,i,0); }
        finish(0,0,100); /* plant and virtual scheduler retained across queue wrap */
    } else if (!strcmp(argv[1],"demo")) {
        init(&f); CHECK(Motion_Enqueue(&f.motion,4096,0,2,2)==MOTION_OK); finish(8192,0,8192);
        printf("demo: events=8192 ticks=%llu final=(8192,0) error=(0,0)\n",(unsigned long long)f.scheduler.snapshot.virtual_ticks);
    } else { CHECK(0); }
    puts("PASS V2.3 integration"); return 0;
}
