#ifndef MOTOR_ANGLE_SPEED_H
#define MOTOR_ANGLE_SPEED_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    volatile int32_t previous_count; /* Legacy Previous_Count; initialize to 0. */
} Motor_AngleSpeedState;

typedef struct {
    float mechanical_angle;
    float electrical_angle;
    float speed_rpm;
} Motor_AngleSpeedResult;

/* Pure extraction of USER/angle.c:60-101, fixed 1 ms sample period.
 * raw_count is the 16-bit CNT read (0..65535); total_count is a separate
 * extended-count sample. Keeping them separate preserves Legacy semantics.
 * No hardware reads, IRQ integration, or R-01 correction are performed. */
void Motor_AngleSpeedCalculate(Motor_AngleSpeedState *state,
                               uint32_t raw_count, int32_t total_count,
                               Motor_AngleSpeedResult *result);

#ifdef __cplusplus
}
#endif

#endif
