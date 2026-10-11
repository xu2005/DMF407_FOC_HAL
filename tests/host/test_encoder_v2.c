#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "encoder_tracker.h"
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while(0)
int main(void)
{
    Encoder_Tracker s;
    int64_t output = 123;
    Encoder_TrackerReset(&s, 0);
    /* Physical displacement oracle; includes both wraps and immediate
     * reversals across the wrap, without relying on an IRQ direction. */
    const int32_t movement[] = { 20000,20000,25535,1,-1,1,17,-18,-20000,-20000,-25535,-1,-1,2,5,-7 };
    int64_t physical=0;
    for (unsigned i=0; i<sizeof(movement)/sizeof(movement[0]); ++i) {
        physical += movement[i];
        uint16_t cnt=(uint16_t)(uint64_t)physical;
        CHECK(Encoder_TrackerUpdate(&s,cnt,&output));
        CHECK(output==physical);
    }
    Encoder_TrackerReset(&s, 65535);
    CHECK(Encoder_TrackerUpdate(&s,0,&output) && output==1);
    CHECK(Encoder_TrackerUpdate(&s,65535,&output) && output==0);
    CHECK(Encoder_TrackerUpdate(&s,65534,&output) && output==-1);
    Encoder_TrackerReset(&s,0);
    output=99;
    CHECK(!Encoder_TrackerUpdate(&s,32768,&output));
    CHECK(output==99 && s.previous==0 && s.total==0);
    s.total=INT64_MAX;
    CHECK(!Encoder_TrackerUpdate(&s,1,&output) && s.total==INT64_MAX);
    s.total=INT64_MIN;
    CHECK(!Encoder_TrackerUpdate(&s,65535,&output) && s.total==INT64_MIN);
    s.total=(int64_t)INT32_MAX+100;
    CHECK(Encoder_TrackerUpdate(&s,1,&output) && output==(int64_t)INT32_MAX+101);
    Encoder_TrackerReset(&s,0);
    CHECK(Encoder_TrackerUpdate(&s,1,&output) && output==1);
    puts("PASS V2 wraps/reversal/restart/int64 limits; movement <32768 per sample is a hardware precondition");
    return 0;
}
