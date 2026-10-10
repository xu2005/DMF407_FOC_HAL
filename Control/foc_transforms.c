#include "foc_transforms.h"

/* SYSTEM/ipark.c:13-23,95-99. Keep signs, precision, and expression order. */
void FOC_Clarke(float u_current, float v_current, FOC_AlphaBetaCurrents *ab_currents)
{
    ab_currents->I_alpha = u_current;
    ab_currents->I_beta = (u_current + 2 * v_current) / 1.73205080757f;
}

void FOC_Park(const FOC_AlphaBetaCurrents *ab_currents, float sinAngle,
              float cosAngle, FOC_DQCurrents *dq_currents)
{
    dq_currents->I_q = -ab_currents->I_alpha * sinAngle + ab_currents->I_beta * cosAngle;
    dq_currents->I_d = ab_currents->I_alpha * cosAngle + ab_currents->I_beta * sinAngle;
}

void FOC_InversePark(const FOC_DQVoltage *dq, FOC_AlphaBetaVoltage *alphaBeta,
                     float sinAngle, float cosAngle)
{
    alphaBeta->alpha = dq->d * cosAngle - dq->q * sinAngle;
    alphaBeta->beta = dq->d * sinAngle + dq->q * cosAngle;
}
