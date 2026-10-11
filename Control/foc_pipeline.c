#include "foc_pipeline.h"
#include <math.h>
#include <string.h>

/* Active Template222_A globals.c / current.c / test.c / SYSTEM/ipark.c.
 * V1 constants, arithmetic precision and 10:1 scheduling are unchanged.
 * Voltage feedback is a diagnostic; Legacy SVPWM still uses fixed Udc=24. */
static const FOC_SpeedPIParameters speed_parameters = { .08f, .0015f, -18, 18 };
static const FOC_CurrentPIParameters current_parameters = { .5f, .05f, 1, .4f, -18, 18 };

void FOC_PipelineInit(FOC_Pipeline *s)
{
    memset(s, 0, sizeof(*s));
    s->speed_ref = 100.0f;
}

void FOC_PipelineAngleTick(FOC_Pipeline *s, uint16_t raw, int32_t total)
{
    Motor_AngleSpeedCalculate(&s->angle_state, raw, total, &s->angle);
}

FOC_TickResult FOC_PipelineControlTick(FOC_Pipeline *s, const FOC_RawSample *raw, int enabled)
{
    float sin_angle = sin(s->angle.electrical_angle);
    float cos_angle = cos(s->angle.electrical_angle);
    s->raw = *raw;
    /* Keep Legacy ADC_REF_VOLTAGE=3.3 (double), intermediate V_adc float. */
    float v_adc = (raw->v / (float)4095) * 3.3;
    float w_adc = (raw->w / (float)4095) * 3.3;
    float bus_adc = (raw->bus / (float)4095) * 3.3;
    s->w_current = (w_adc - 1.250) / 0.12;
    s->v_current = (v_adc - 1.250) / 0.12;
    s->u_current = -s->w_current - s->v_current;
    s->bus_voltage = bus_adc * 25;
    FOC_Clarke(s->u_current, s->v_current, &s->ab_current);
    FOC_Park(&s->ab_current, sin_angle, cos_angle, &s->dq_current);
    s->q_current = s->dq_current.I_q; /* Legacy park diagnostic mirror. */
    ++s->current_cycle_counter;
    if (!enabled) {
        return FOC_TICK_DISABLED; /* Preserve PI state and divider, as Legacy. */
    }
    if (s->current_cycle_counter >= 10) {
        s->motor_x_position = s->motor_y_position = 0;
        s->iq_ref = FOC_SpeedPIUpdate(&s->speed_pi, &speed_parameters,
                                     s->speed_ref, s->angle.speed_rpm);
        s->current_cycle_counter = 0;
        ++s->speed_updates;
    }
    FOC_CurrentPIUpdate(&s->current_pi, &current_parameters, s->id_ref, s->iq_ref,
                        &s->dq_current, &s->dq_voltage);
    FOC_InversePark(&s->dq_voltage, &s->ab_voltage, sin_angle, cos_angle);
    return FOC_SVPWMCalculate(&s->ab_voltage, &s->pwm) ? FOC_TICK_VALID : FOC_TICK_INVALID;
}
