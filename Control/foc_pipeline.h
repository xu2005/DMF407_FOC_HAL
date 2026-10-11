#ifndef FOC_PIPELINE_H
#define FOC_PIPELINE_H
#include <stdint.h>
#include "motor_angle_speed.h"
#include "foc_pi.h"
#include "foc_svpwm.h"

/* No HAL/global writes. Caller supplies coherent ADC/encoder snapshots.
 * Stage fields are available for deterministic replay, not timing evidence. */
typedef struct {
    uint16_t bus, v, w;
} FOC_RawSample;
typedef struct {
    Motor_AngleSpeedState angle_state;
    Motor_AngleSpeedResult angle;
    FOC_RawSample raw;
    float u_current, v_current, w_current, bus_voltage;
    FOC_AlphaBetaCurrents ab_current;
    FOC_DQCurrents dq_current;
    FOC_SpeedPIState speed_pi;
    FOC_CurrentPIState current_pi;
    FOC_DQVoltage dq_voltage;
    FOC_AlphaBetaVoltage ab_voltage;
    FOC_SVPWMResult pwm;
    float speed_ref, id_ref, iq_ref, q_current;
    uint32_t current_cycle_counter, speed_updates;
    int32_t motor_x_position, motor_y_position;
    int32_t id_show, iq_show; /* Legacy ARM long diagnostic mirrors. */
} FOC_Pipeline;
typedef enum { FOC_TICK_INVALID = -1, FOC_TICK_DISABLED = 0, FOC_TICK_VALID = 1 } FOC_TickResult;
void FOC_PipelineInit(FOC_Pipeline *s);
void FOC_PipelineAngleTick(FOC_Pipeline *s, uint16_t raw, int32_t total);
FOC_TickResult FOC_PipelineControlTick(FOC_Pipeline *s, const FOC_RawSample *raw, int enabled);
#endif
