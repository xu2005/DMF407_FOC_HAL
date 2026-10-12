/* Standalone host demo. Deliberately excluded from the firmware build. */
#include "motion_virtual_plant.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
static Motion_Profile profile;
static Motion motion;
static Motion_VirtualPlant plant;
static Motion_Scheduler scheduler;
static const char *state_name(Motion_SchedulerState state)
{
    switch (state) {
    case SCHED_ACTIVE: return "ACTIVE";
    case SCHED_COMPLETE: return "COMPLETE";
    case SCHED_CANCELLED: return "CANCELLED";
    case SCHED_FAULT: return "FAULT";
    case SCHED_BUDGET: return "BUDGET";
    default: return "INVALID";
    }
}
int main(int argc,char **argv)
{
    if (argc!=3) { fprintf(stderr,"usage: motion_v23_demo demo|mixed|drop|reject-x|reject-y|lag|encoder|long|zero|cancel output.csv\n"); return 2; }
    Motion_ProfileConfig config=Motion_ProfileLegacyConfig();
    Motion_VirtualPlantConfig pc=Motion_VirtualPlantIdealConfig();
    int32_t tx=8192,ty=0;
    int mixed=!strcmp(argv[1],"mixed");
    int cancel=!strcmp(argv[1],"cancel");
    if (mixed) { tx=0; ty=0; }
    else if (!strcmp(argv[1],"drop")) { tx=40; ty=15; pc.drop_sequence=3; }
    else if (!strcmp(argv[1],"reject-x")) { tx=40; pc.reject_sequence=3; }
    else if (!strcmp(argv[1],"reject-y")) { tx=0; ty=40; pc.reject_sequence=3; pc.reject_axis=MOTION_AXIS_Y; }
    else if (!strcmp(argv[1],"lag")) { tx=40; ty=15; pc.feedback_lag_events=4; }
    else if (!strcmp(argv[1],"encoder")) {
        tx=-40; ty=15; pc.counts_per_step[0]=4; pc.counts_per_step[1]=8;
        pc.quantum_counts[0]=3; pc.quantum_counts[1]=5; pc.bias_counts[0]=2; pc.bias_counts[1]=-3;
    }
    else if (!strcmp(argv[1],"long")) { tx=12000; ty=0; }
    else if (!strcmp(argv[1],"zero")) { tx=0; ty=0; }
    else if (strcmp(argv[1],"demo") && !cancel) { fprintf(stderr,"unknown scenario\n"); return 2; }
    if (Motion_ProfileBuild(&profile,&config)!=MOTION_PROFILE_OK ||
        Motion_Init(&motion,&profile,0,0)!=MOTION_OK || !Motion_VirtualPlantInit(&plant,&pc,0,0)) { return 3; }
    if (mixed) {
        const int32_t points[][2]={{0,0},{40,15},{40,15},{-20,30},{-30,-20},{10,-10},{0,0}};
        for (unsigned i=0;i<sizeof(points)/sizeof(points[0]);++i) {
            if (Motion_Enqueue(&motion,points[i][0],points[i][1],1,1)!=MOTION_OK) { return 3; }
        }
    } else if (Motion_Enqueue(&motion,tx,ty,1,1)!=MOTION_OK) { return 3; }
    if (Motion_Start(&motion)!=MOTION_OK ||
        !Motion_SchedulerInit(&scheduler,&motion,Motion_VirtualPlantSink,&plant,0,20000)) { return 3; }
    FILE *csv=fopen(argv[2],"w");
    if (!csv) { perror("CSV"); return 4; }
    fprintf(csv,"event_id,virtual_ticks,axis,direction,period,target_x,target_y,planned_x,planned_y,confirmed_x,confirmed_y,plant_x,plant_y,encoder_x,encoder_y,feedback_x,feedback_y,error_x,error_y,confirmed_error_x,confirmed_error_y,accepted,state\n");
    while (scheduler.snapshot.state==SCHED_ACTIVE) {
        uint64_t before=scheduler.snapshot.dispatched;
        (void)Motion_SchedulerStep(&scheduler);
        if (scheduler.snapshot.dispatched==before) { break; }
        if (cancel && scheduler.snapshot.dispatched==8) { Motion_SchedulerCancel(&scheduler); }
        Motion_SchedulerSnapshot s; Motion_VirtualPlantSnapshot p;
        Motion_SchedulerRead(&scheduler,&s); Motion_VirtualPlantRead(&plant,&s,&p);
        fprintf(csv,"%" PRIu64 ",%" PRIu64 ",%c,%d,%u,%d,%d,%d,%d,%d,%d,%d,%d,%" PRId64 ",%" PRId64 ",%.9g,%.9g,%.9g,%.9g,%.9g,%.9g,%d,%s\n",
                s.event.sequence,s.virtual_ticks,s.event.axis==MOTION_AXIS_X ? 'X' : 'Y',s.event.direction,s.event.period,
                tx,ty,s.planned_x,s.planned_y,s.confirmed_x,s.confirmed_y,p.plant_x,p.plant_y,p.encoder_x,p.encoder_y,
                p.feedback_x,p.feedback_y,p.planned_error_x,p.planned_error_y,p.confirmed_error_x,p.confirmed_error_y,s.accepted,state_name(s.state));
    }
    int io_error=ferror(csv); if (fclose(csv)!=0) { io_error=1; }
    if (io_error) { fprintf(stderr,"CSV write failed\n"); return 4; }
    printf("scenario=%s events=%" PRIu64 " accepted=%" PRIu64 " ticks=%" PRIu64 " state=%s error=%d dropped=%" PRIu64 "\n",
           argv[1],scheduler.snapshot.dispatched,plant.snapshot.accepted,scheduler.snapshot.virtual_ticks,
           state_name(scheduler.snapshot.state),scheduler.snapshot.error,plant.snapshot.dropped);
    int expected_fault=!strcmp(argv[1],"reject-x") || !strcmp(argv[1],"reject-y");
    return scheduler.snapshot.state==(expected_fault ? SCHED_FAULT : cancel ? SCHED_CANCELLED : SCHED_COMPLETE) ? 0 : 5;
}
