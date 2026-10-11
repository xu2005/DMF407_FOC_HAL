#include "motion_profile.h"
#include "vectors/motion_profile_legacy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static Motion_Profile profile;
int main(int argc, char **argv)
{
    CHECK(argc == 2);
    Motion_ProfileConfig config = Motion_ProfileLegacyConfig();
    if (!strcmp(argv[1],"vectors")) {
        for (size_t i=0; i<sizeof(legacy_profiles)/sizeof(legacy_profiles[0]); ++i) {
            config.show_speed = legacy_profiles[i].speed;
            CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_OK);
            CHECK(profile.valid && profile.dec_len == legacy_profiles[i].count);
            CHECK(!memcmp(profile.period,legacy_profiles[i].period,
                          (profile.dec_len+1)*sizeof(uint16_t)));
            CHECK(profile.period_min == profile.period[profile.dec_len]);
            CHECK(profile.period_max == profile.period[0]);
        }
    } else if (!strcmp(argv[1],"input")) {
        CHECK(Motion_ProfileBuild(NULL,&config) == MOTION_PROFILE_INPUT);
        CHECK(Motion_ProfileBuild(&profile,NULL) == MOTION_PROFILE_INPUT && !profile.valid);
        config.position_limit=0;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.show_speed=-1;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.step_degrees=NAN;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.step_degrees=1e300;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.duration_ms=INFINITY;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.flexibility=-1;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
        config=Motion_ProfileLegacyConfig(); config.subdivision=0;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_INPUT);
    } else if (!strcmp(argv[1],"clamps")) {
        Motion_Profile reference;
        config.vmin_rpm=-10; config.duration_ms=1000;
        CHECK(Motion_ProfileBuild(&profile,&config) == MOTION_PROFILE_OK);
        config=Motion_ProfileLegacyConfig();
        CHECK(Motion_ProfileBuild(&reference,&config) == MOTION_PROFILE_OK);
        CHECK(profile.dec_len==reference.dec_len);
        CHECK(!memcmp(profile.period,reference.period,(profile.dec_len+1)*sizeof(uint16_t)));
        config.vmin_rpm=100; config.duration_ms=-100;
        CHECK(Motion_ProfileBuild(&profile,&config)==MOTION_PROFILE_OK);
        config.vmin_rpm=10; config.duration_ms=30;
        CHECK(Motion_ProfileBuild(&reference,&config)==MOTION_PROFILE_OK);
        CHECK(profile.dec_len==reference.dec_len);
        CHECK(!memcmp(profile.period,reference.period,(profile.dec_len+1)*sizeof(uint16_t)));
    } else if (!strcmp(argv[1],"capacity")) {
        struct { uint32_t before; Motion_Profile p; uint32_t after; } guard;
        guard.before=0x12345678; guard.after=0x87654321;
        config.show_speed=1627;
        CHECK(Motion_ProfileBuild(&guard.p,&config)==MOTION_PROFILE_OK);
        CHECK(guard.p.valid && guard.p.dec_len==MOTION_ARRAY_LENGTH);
        CHECK(guard.p.period_min==guard.p.period[MOTION_ARRAY_LENGTH]);
        CHECK(guard.before==0x12345678 && guard.after==0x87654321);
        config.show_speed=1628;
        CHECK(Motion_ProfileBuild(&guard.p,&config)==MOTION_PROFILE_CAPACITY);
        CHECK(!guard.p.valid && guard.p.dec_len==0);
        CHECK(guard.before==0x12345678 && guard.after==0x87654321);
        config.show_speed=100000;
        CHECK(Motion_ProfileBuild(&guard.p,&config)==MOTION_PROFILE_CAPACITY);
        CHECK(!guard.p.valid && guard.p.dec_len==0);
        CHECK(guard.before==0x12345678 && guard.after==0x87654321);
        CHECK(Motion_ProfileBuild(&guard.p,NULL)==MOTION_PROFILE_INPUT);
    } else { CHECK(0); }
    puts("PASS motion profile");
    return 0;
}
