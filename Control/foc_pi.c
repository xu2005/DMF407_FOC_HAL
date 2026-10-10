#include "foc_pi.h"

/* Active Legacy SYSTEM/ipark.c:25-92; globals become explicit inputs/state. */
static void clamp(float *value, float minimum, float maximum)
{
    if (*value > maximum) {
        *value = maximum;
    } else if (*value < minimum) {
        *value = minimum;
    }
}

float FOC_PositionCalculate(int32_t position_ref_x, int32_t position_actual,
                            int32_t position_feed, float kp, float kff, int32_t limit)
{
    float err_position = position_ref_x - position_actual;
    float speed_ref = kp * err_position + kff * position_feed;
    clamp(&speed_ref, -limit, limit);
    return speed_ref;
}

float FOC_SpeedPIUpdate(FOC_SpeedPIState *state, const FOC_SpeedPIParameters *parameters,
                        float speed_ref, float speed_actual)
{
    float err_speed = speed_ref - speed_actual;
    float sum_speed_local = state->sum_speed;
    sum_speed_local += parameters->ki * err_speed;
    clamp(&sum_speed_local, parameters->minimum, parameters->maximum);
    float iq_ref = parameters->kp * err_speed + sum_speed_local;
    clamp(&iq_ref, parameters->minimum, parameters->maximum);
    state->sum_speed = sum_speed_local;
    return iq_ref;
}

void FOC_CurrentPIUpdate(FOC_CurrentPIState *state, const FOC_CurrentPIParameters *parameters,
                         float id_ref, float iq_ref, const FOC_DQCurrents *measured,
                         FOC_DQVoltage *output)
{
    float err_d = id_ref - measured->I_d;
    float sum_d_local = state->sum_d;
    float err_q = iq_ref - measured->I_q;
    float sum_q_local = state->sum_q;

    sum_d_local += err_d;
    clamp(&sum_d_local, parameters->minimum, parameters->maximum);
    output->d = parameters->id_kp * err_d + parameters->id_ki * sum_d_local;
    clamp(&output->d, parameters->minimum, parameters->maximum);
    state->sum_d = sum_d_local;

    sum_q_local += err_q;
    clamp(&sum_q_local, parameters->minimum, parameters->maximum);
    output->q = parameters->iq_kp * err_q + parameters->iq_ki * sum_q_local;
    clamp(&output->q, parameters->minimum, parameters->maximum);
    state->sum_q = sum_q_local;
}
