#include "v23_fixture.h"
#include "vectors/motion_xy_legacy.h"
static Fixture a,b;
static void hash(uint64_t *h,uint32_t value)
{
    for (int i=0;i<4;++i) { *h^=(value>>(8*i))&255; *h*=UINT64_C(1099511628211); }
}
static int recursive(void *context,const Motion_Event *e,uint64_t ticks)
{
    Fixture *f=context;
    CHECK(Motion_SchedulerStep(&f->scheduler)==SCHED_BUSY);
    CHECK(Motion_SchedulerRun(&f->scheduler,2)==SCHED_BUSY);
    return Motion_VirtualPlantSink(&f->plant,e,ticks);
}
int main(int argc,char **argv)
{
    CHECK(argc==2);
    if (!strcmp(argv[1],"determinism")) {
        for (size_t v=0;v<sizeof(legacy_xy)/sizeof(legacy_xy[0]);++v) {
            init(&a); init(&b);
            enqueue(&a,legacy_xy[v].x*2,legacy_xy[v].y*2);
            enqueue(&b,legacy_xy[v].x*2,legacy_xy[v].y*2);
            start(&a,0,20000); start(&b,0,20000);
            uint64_t ticks=0,h=UINT64_C(14695981039346656037);
            while (a.motion.state==MOTION_RUNNING) {
                Motion_SchedulerResult result=Motion_SchedulerStep(&a.scheduler);
                if (result==SCHED_DONE) { break; }
                CHECK(result==SCHED_EVENT);
                Motion_Event e=a.scheduler.snapshot.event, old;
                CHECK(Motion_Next(&b.motion,&old)==MOTION_OK);
                CHECK(e.sequence==old.sequence && e.axis==old.axis && e.direction==old.direction);
                CHECK(e.period==old.period && e.x==old.x && e.y==old.y);
                CHECK(Motion_Confirm(&b.motion,old.sequence,1)==MOTION_OK);
                ticks+=old.period;
                CHECK(a.scheduler.snapshot.virtual_ticks==ticks);
                hash(&h,e.axis); hash(&h,e.period); hash(&h,(uint32_t)e.x); hash(&h,(uint32_t)e.y);
            }
            CHECK(a.scheduler.snapshot.dispatched==legacy_xy[v].count && h==legacy_xy[v].digest);
            init(&b); enqueue(&b,legacy_xy[v].x*2,legacy_xy[v].y*2); start(&b,0,20000);
            CHECK(Motion_SchedulerRun(&b.scheduler,20000)==SCHED_DONE);
            CHECK(b.scheduler.snapshot.virtual_ticks==ticks && b.scheduler.snapshot.dispatched==a.scheduler.snapshot.dispatched);
            CHECK(b.scheduler.snapshot.state==a.scheduler.snapshot.state);
            CHECK(b.motion.confirmed_x==a.motion.confirmed_x && b.motion.confirmed_y==a.motion.confirmed_y);
        }
    } else if (!strcmp(argv[1],"budget")) {
        init(&a); enqueue(&a,10001,0); start(&a,0,10001);
        CHECK(Motion_SchedulerRun(&a.scheduler,0)==SCHED_LIMIT && a.plant.snapshot.received==0);
        CHECK(Motion_SchedulerRun(&a.scheduler,100)==SCHED_LIMIT && a.scheduler.snapshot.state==SCHED_ACTIVE);
        CHECK(Motion_SchedulerRun(&a.scheduler,9901)==SCHED_DONE && a.plant.snapshot.accepted==10001);
        init(&a); enqueue(&a,10001,0); start(&a,0,10000);
        CHECK(Motion_SchedulerRun(&a.scheduler,20000)==SCHED_LIMIT && a.scheduler.snapshot.state==SCHED_BUDGET);
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_LIMIT && a.plant.snapshot.accepted==10000 && !a.motion.pending);
    } else if (!strcmp(argv[1],"stop")) {
        init(&a); enqueue(&a,10,10); start(&a,0,100);
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_EVENT);
        Motion_SchedulerCancel(&a.scheduler);
        CHECK(Motion_SchedulerRun(&a.scheduler,100)==SCHED_STOPPED && a.plant.snapshot.received==1);
        init(&a); enqueue(&a,10,0); start(&a,0,100); Motion_Fault(&a.motion);
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.plant.snapshot.received==0);
    } else if (!strcmp(argv[1],"invalid")) {
        init(&a); enqueue(&a,1,0); start(&a,UINT64_MAX,10);
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.scheduler.snapshot.error==SCHED_TIME_OVERFLOW);
        CHECK(a.plant.snapshot.received==0 && a.scheduler.snapshot.virtual_ticks==UINT64_MAX && !a.motion.pending);
        init(&a); enqueue(&a,1,0); start(&a,0,10);
        memset(a.profile.period,0,sizeof(a.profile.period));
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.scheduler.snapshot.error==SCHED_ZERO_PERIOD);
        CHECK(a.plant.snapshot.received==0);
        init(&a); enqueue(&a,1,0); start(&a,0,10); a.motion.planned_events=9;
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.scheduler.snapshot.error==SCHED_SEQUENCE);
        CHECK(a.plant.snapshot.received==0);
        init(&a); enqueue(&a,1,0); start(&a,0,10);
        a.motion.planned_events=a.motion.confirmed_events=UINT64_MAX;
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.scheduler.snapshot.error==SCHED_SEQUENCE);
        CHECK(a.plant.snapshot.received==0 && !a.motion.pending);
        init(&a); enqueue(&a,1,0); start(&a,0,10);
        Motion_Event e; CHECK(Motion_Next(&a.motion,&e)==MOTION_OK);
        CHECK(!Motion_SchedulerInit(&b.scheduler,&a.motion,Motion_VirtualPlantSink,&a.plant,0,10));
        CHECK(Motion_SchedulerStep(&a.scheduler)==SCHED_ERROR && a.plant.snapshot.received==0);
        CHECK(!Motion_SchedulerInit(&b.scheduler,&a.motion,Motion_VirtualPlantSink,&a.plant,0,0));
    } else if (!strcmp(argv[1],"reentry")) {
        init(&a); enqueue(&a,10,0); start(&a,0,20);
        a.scheduler.sink=recursive; a.scheduler.sink_context=&a;
        CHECK(Motion_SchedulerRun(&a.scheduler,20)==SCHED_DONE && a.plant.snapshot.received==10);
    } else { CHECK(0); }
    puts("PASS scheduler"); return 0;
}
