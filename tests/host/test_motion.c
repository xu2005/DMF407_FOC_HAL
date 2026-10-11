#include "motion.h"
#include "vectors/motion_xy_legacy.h"
#include "vectors/motion_batch_legacy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static Motion_Profile profile;
static Motion motion;
static uint64_t digest;
static void hash(uint32_t value)
{
    for (int i=0;i<4;i++) { digest^=(value>>(8*i))&255; digest*=UINT64_C(1099511628211); }
}
static void init(void)
{
    Motion_ProfileConfig config=Motion_ProfileLegacyConfig();
    CHECK(Motion_ProfileBuild(&profile,&config)==MOTION_PROFILE_OK);
    CHECK(Motion_Init(&motion,&profile,0,0)==MOTION_OK);
    digest=UINT64_C(14695981039346656037);
}
static unsigned finish(int compare_small)
{
    unsigned count=0;
    while (motion.state==MOTION_RUNNING) {
        Motion_Event event;
        Motion_Result result=Motion_Next(&motion,&event);
        if (result==MOTION_EMPTY) { CHECK(motion.state==MOTION_COMPLETE); break; }
        CHECK(result==MOTION_OK);
        CHECK(event.sequence==motion.planned_events);
        CHECK(event.period>=30 && event.period<=26715);
        CHECK(motion.profile_index<=profile.dec_len);
        CHECK(abs(event.x-motion.confirmed_x)+abs(event.y-motion.confirmed_y)==1);
        CHECK(Motion_Next(&motion,&event)==MOTION_BUSY);
        if (compare_small) {
            CHECK(count<sizeof(legacy_xy_3_2)/sizeof(legacy_xy_3_2[0]));
            CHECK((uint32_t)event.axis==legacy_xy_3_2[count].axis);
            CHECK(event.period==legacy_xy_3_2[count].period);
            CHECK(event.x==legacy_xy_3_2[count].x && event.y==legacy_xy_3_2[count].y);
        }
        hash(event.axis); hash(event.period); hash((uint32_t)event.x); hash((uint32_t)event.y);
        CHECK(Motion_Confirm(&motion,event.sequence,1)==MOTION_OK);
        CHECK(++count<30000);
    }
    CHECK(motion.confirmed_events==motion.planned_events);
    CHECK(motion.confirmed_x==motion.target_x && motion.confirmed_y==motion.target_y);
    return count;
}
int main(int argc,char **argv)
{
    CHECK(argc==2); init();
    if (!strcmp(argv[1],"vectors")) {
        for (size_t i=0;i<sizeof(legacy_xy)/sizeof(legacy_xy[0]);i++) {
            init();
            CHECK(Motion_Enqueue(&motion,legacy_xy[i].x,legacy_xy[i].y,2,2)==MOTION_OK);
            CHECK(Motion_Start(&motion)==MOTION_OK);
            CHECK(finish(legacy_xy[i].x==3 && legacy_xy[i].y==2)==legacy_xy[i].count);
            CHECK(digest==legacy_xy[i].digest);
            CHECK(motion.profile_index==legacy_xy[i].final_index);
        }
    } else if (!strcmp(argv[1],"batch_vectors")) {
        for (size_t i=0;i<sizeof(legacy_batches)/sizeof(legacy_batches[0]);i++) {
            init();
            for (uint32_t j=0;j<legacy_batches[i].segments;j++) {
                CHECK(Motion_Enqueue(&motion,legacy_batches[i].points[j][0],
                      legacy_batches[i].points[j][1],2,2)==MOTION_OK);
            }
            CHECK(Motion_Start(&motion)==MOTION_OK);
            CHECK(finish(0)==legacy_batches[i].count);
            CHECK(digest==legacy_batches[i].digest);
            CHECK(motion.profile_index==legacy_batches[i].final_index);
        }
    } else if (!strcmp(argv[1],"quadrants")) {
        for (int sx=-1;sx<=1;sx+=2) {
            for (int sy=-1;sy<=1;sy+=2) {
                init(); CHECK(Motion_Enqueue(&motion,sx*3,sy*2,2,2)==MOTION_OK);
                CHECK(Motion_Start(&motion)==MOTION_OK);
                unsigned i=0;
                while (motion.state==MOTION_RUNNING) {
                    Motion_Event e; CHECK(Motion_Next(&motion,&e)==MOTION_OK);
                    CHECK(e.x==sx*legacy_xy_3_2[i].x && e.y==sy*legacy_xy_3_2[i].y);
                    CHECK(e.direction==(e.axis==MOTION_AXIS_X ? sx : sy));
                    CHECK(e.period==legacy_xy_3_2[i].period);
                    CHECK(Motion_Confirm(&motion,e.sequence,1)==MOTION_OK); ++i;
                }
                CHECK(i==10);
            }
        }
    } else if (!strcmp(argv[1],"queue")) {
        CHECK(Motion_Start(&motion)==MOTION_EMPTY);
        for (unsigned batch=0;batch<3;batch++) {
            for (unsigned i=0;i<MOTION_DATA_LEN;i++) {
                CHECK(Motion_Enqueue(&motion,(int32_t)(batch*100+i+1),0,1,1)==MOTION_OK);
            }
            CHECK(Motion_Enqueue(&motion,999,999,1,1)==MOTION_FULL);
            CHECK(Motion_Start(&motion)==MOTION_OK);
            CHECK(Motion_Enqueue(&motion,999,999,1,1)==MOTION_BUSY);
            CHECK(finish(0)==100);
            CHECK(Motion_Rearm(&motion)==MOTION_OK);
        }
        /* 37 consumed slots force an actually wrapped next batch. */
        for (int i=0;i<37;i++) CHECK(Motion_Enqueue(&motion,301+i,0,1,1)==MOTION_OK);
        CHECK(Motion_Start(&motion)==MOTION_OK); CHECK(finish(0)==37);
        CHECK(Motion_Rearm(&motion)==MOTION_OK && motion.head==37);
        for (int i=0;i<100;i++) CHECK(Motion_Enqueue(&motion,338+i,0,1,1)==MOTION_OK);
        CHECK(Motion_Start(&motion)==MOTION_OK); CHECK(finish(0)==100);
    } else if (!strcmp(argv[1],"boundaries")) {
        CHECK(Motion_Enqueue(&motion,1,0,NAN,1)==MOTION_INVALID);
        CHECK(Motion_Enqueue(&motion,INT32_MAX,0,2,1)==MOTION_INVALID);
        CHECK(Motion_Enqueue(&motion,1,1,INFINITY,1)==MOTION_INVALID);
        CHECK(motion.count==0 && motion.target_x==0);
        CHECK(Motion_Enqueue(&motion,3,-3,0.5,0.5)==MOTION_OK);
        CHECK(Motion_Start(&motion)==MOTION_OK); CHECK(finish(0)==2);
        CHECK(motion.confirmed_x==1 && motion.confirmed_y==-1);
        CHECK(Motion_Init(&motion,&profile,INT32_MIN,INT32_MAX)==MOTION_OK);
        CHECK(Motion_Enqueue(&motion,INT32_MAX,INT32_MIN,1,1)==MOTION_OK);
        CHECK(motion.queue[0].dx==UINT32_MAX && motion.queue[0].dy==UINT32_MAX);
        CHECK(Motion_Start(&motion)==MOTION_OK);
        for (int i=0;i<10;i++) {
            Motion_Event e; CHECK(Motion_Next(&motion,&e)==MOTION_OK);
            CHECK(Motion_Confirm(&motion,e.sequence,1)==MOTION_OK);
        }
        Motion_Cancel(&motion); CHECK(motion.state==MOTION_CANCELLED);
        profile.valid=0; CHECK(Motion_Init(&motion,&profile,0,0)==MOTION_INVALID);
    } else if (!strcmp(argv[1],"cancel_fault")) {
        CHECK(Motion_Enqueue(&motion,10,10,1,1)==MOTION_OK);
        CHECK(Motion_Start(&motion)==MOTION_OK);
        Motion_Event e;
        CHECK(Motion_Next(&motion,&e)==MOTION_OK);
        CHECK(Motion_Confirm(&motion,e.sequence+1,1)==MOTION_INVALID);
        CHECK(motion.confirmed_events==0 && motion.pending);
        Motion_Cancel(&motion);
        CHECK(motion.state==MOTION_CANCELLED && motion.confirmed_x==0 && motion.confirmed_y==0);
        CHECK(Motion_Confirm(&motion,e.sequence,1)==MOTION_INVALID);
        CHECK(Motion_Next(&motion,&e)==MOTION_TERMINAL);
        CHECK(Motion_Rearm(&motion)==MOTION_OK && motion.planned_x==0);
        CHECK(Motion_Enqueue(&motion,-2,-1,1,1)==MOTION_OK); CHECK(Motion_Start(&motion)==MOTION_OK);
        CHECK(Motion_Next(&motion,&e)==MOTION_OK);
        CHECK(Motion_Confirm(&motion,e.sequence,0)==MOTION_TERMINAL);
        CHECK(motion.state==MOTION_FAULT && motion.confirmed_events==0);
        Motion_Cancel(&motion); CHECK(motion.state==MOTION_FAULT);
        CHECK(Motion_Rearm(&motion)==MOTION_BUSY);
        CHECK(Motion_Init(&motion,&profile,0,0)==MOTION_OK);
    } else if (!strcmp(argv[1],"segments")) {
        const int points[][2]={{0,0},{2,1},{2,1},{-2,1},{-2,-2},{0,0},{0,0}};
        for (size_t i=0;i<sizeof(points)/sizeof(points[0]);i++) {
            CHECK(Motion_Enqueue(&motion,points[i][0],points[i][1],1,1)==MOTION_OK);
        }
        CHECK(Motion_Start(&motion)==MOTION_OK); CHECK(finish(0)==14);
        CHECK(motion.state==MOTION_COMPLETE && motion.count==0);
    } else if (!strcmp(argv[1],"index")) {
        CHECK(Motion_Enqueue(&motion,10000,0,1,1)==MOTION_OK);
        CHECK(Motion_Start(&motion)==MOTION_OK);
        int terminal_seen=0;
        while (motion.state==MOTION_RUNNING) {
            Motion_Event e; CHECK(Motion_Next(&motion,&e)==MOTION_OK);
            if (motion.profile_index==profile.dec_len) {
                terminal_seen=1; CHECK(e.period==profile.period_min);
            }
            CHECK(motion.profile_index<=profile.dec_len);
            CHECK(Motion_Confirm(&motion,e.sequence,1)==MOTION_OK);
        }
        CHECK(terminal_seen && motion.profile_index==1);
    } else { CHECK(0); }
    puts("PASS motion events"); return 0;
}
