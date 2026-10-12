#ifndef V23_FIXTURE_H
#define V23_FIXTURE_H
#include "motion_virtual_plant.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while (0)
typedef struct {
    Motion_Profile profile;
    Motion motion;
    Motion_VirtualPlant plant;
    Motion_Scheduler scheduler;
} Fixture;
static inline void init(Fixture *f)
{
    Motion_ProfileConfig c=Motion_ProfileLegacyConfig();
    Motion_VirtualPlantConfig p=Motion_VirtualPlantIdealConfig();
    CHECK(Motion_ProfileBuild(&f->profile,&c)==MOTION_PROFILE_OK);
    CHECK(Motion_Init(&f->motion,&f->profile,0,0)==MOTION_OK);
    CHECK(Motion_VirtualPlantInit(&f->plant,&p,0,0));
}
static inline void enqueue(Fixture *f,int32_t x,int32_t y)
{
    CHECK(Motion_Enqueue(&f->motion,x,y,1,1)==MOTION_OK);
}
static inline void start(Fixture *f,uint64_t ticks,uint64_t budget)
{
    CHECK(Motion_Start(&f->motion)==MOTION_OK);
    CHECK(Motion_SchedulerInit(&f->scheduler,&f->motion,Motion_VirtualPlantSink,&f->plant,ticks,budget));
}
static inline Motion_VirtualPlantSnapshot feedback(Fixture *f)
{
    Motion_VirtualPlantSnapshot p;
    Motion_VirtualPlantRead(&f->plant,&f->scheduler.snapshot,&p);
    return p;
}
#endif
