#include "motion_profile.h"
#include <math.h>
#include <limits.h>

Motion_ProfileConfig Motion_ProfileLegacyConfig(void)
{
    Motion_ProfileConfig config = {200, 1000, 1.8, 1.0, 300.0, 16, 4.0f};
    return config;
}

Motion_ProfileResult Motion_ProfileBuild(Motion_Profile *out, const Motion_ProfileConfig *config)
{
    if (!out) { return MOTION_PROFILE_INPUT; }
    out->valid = 0;
    out->dec_len = 0;
    out->period_min = out->period_max = 0;
    if (!config || config->show_speed < 0 || config->position_limit <= 0 ||
        !isfinite(config->step_degrees) || config->step_degrees <= 0.0 ||
        config->subdivision == 0 || !isfinite(config->vmin_rpm) ||
        !isfinite(config->duration_ms) || !isfinite(config->flexibility) ||
        config->flexibility < 0.0f) { return MOTION_PROFILE_INPUT; }
    float stepan = (float)(360.0 / config->step_degrees);
    double ppr_value = 60*1000*1000.0 / (stepan * config->subdivision);
    if (!isfinite(ppr_value) || ppr_value < 1.0 || ppr_value > INT_MAX) {
        return MOTION_PROFILE_INPUT;
    }
    int PPR = (int)ppr_value;
    double cpuhz = 1.0/0.13925;
    double Vmin = config->vmin_rpm;
    double Tmax = config->duration_ms;
    if (Vmin < 1.0) { Vmin = 1.0; }
    if (Vmin > 10.0) { Vmin = 10.0; }
    if (Tmax < 30.0) { Tmax = 30.0; }
    if (Tmax > 300.0) { Tmax = 300.0; }
    double Vmax = (double)config->show_speed*230.0/config->position_limit;
    if (Vmax <= Vmin+1.0) { Vmax = Vmin+1.0; }
    int num = (int)(Tmax*0.5);
    float flexible = config->flexibility;
    float T = 0;
    uint32_t count = 0;
    int period = 0;
    while (T < Tmax) {
        if (count == MOTION_ARRAY_LENGTH) { return MOTION_PROFILE_CAPACITY; }
        float melo = flexible*(T-num)/num;
        float deno = (float)(1.0/(1+expf(-melo)));
        float v = (float)((Vmax-Vmin)*deno+Vmin);
        double value = (PPR/v-2.45-14)*cpuhz;
        /* Clamp before cast to avoid undefined float-to-int overflow.
         * For original defined inputs the integer result is unchanged. */
        if (value > 26715.0) { period = 26715; }
        else if (value < 31.0) { period = 31; }
        else { period = (int)value; }
        out->period[count++] = (uint16_t)period;
        float fre = (float)PPR/v;
        float next = (float)(T+fre/1000.0);
        if (!isfinite(next) || next <= T) { return MOTION_PROFILE_INPUT; }
        T = next;
    }
    out->period[count] = (uint16_t)(period-1);
    out->dec_len = count;
    out->period_min = out->period[count];
    out->period_max = out->period[0];
    out->valid = 1;
    return MOTION_PROFILE_OK;
}
