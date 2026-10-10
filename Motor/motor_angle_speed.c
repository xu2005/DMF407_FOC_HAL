#include "motor_angle_speed.h"
#include <math.h>

/* Active Template222_A/USER/drv_config.h and angle.c:11. */
#define ENCODER_RESOLUTION 4000
#define PI 3.1415926f
#define POLE_PAIRS 4
static const float mechanical_angle_offset = 0.546637118;

void Motor_AngleSpeedCalculate(Motor_AngleSpeedState *state,
                               uint32_t raw_count, int32_t total_count,
                               Motor_AngleSpeedResult *result)
{
    /* Legacy u32 - int32 produces u32, then converts to ARM's 32-bit long.
     * Use int32_t so an LP64 host cannot silently turn Count into 64 bits. */
    int32_t Count = (int32_t)(raw_count - (uint32_t)state->previous_count);
    if (Count < -32768) {
        Count = Count + 65536;
    } else if (Count > 32767) {
        Count -= 65536;
    }
    state->previous_count = (int32_t)raw_count;

    float delta_time = 0.001f;
    float mechanical_angle = ((float)total_count / (float)ENCODER_RESOLUTION) * 2.0f * PI;
    /* Retain double fmod followed by float assignment; do not use fmodf. */
    mechanical_angle = fmod(mechanical_angle, 2.0f * PI);
    if (mechanical_angle < 0) {
        mechanical_angle += 2.0f * PI;
    }
    mechanical_angle = mechanical_angle - mechanical_angle_offset;
    if (mechanical_angle < 0) {
        mechanical_angle += 2.0f * PI;
    }
    result->mechanical_angle = mechanical_angle;
    result->electrical_angle = mechanical_angle * POLE_PAIRS;
    result->electrical_angle = fmod(result->electrical_angle, 2.0f * PI);
    if (result->electrical_angle < 0) {
        result->electrical_angle += 2.0f * PI;
    }
    result->speed_rpm = (float)Count / (float)ENCODER_RESOLUTION * 60.0f / delta_time;
}
