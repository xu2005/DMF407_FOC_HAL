#ifndef FOC_TRANSFORMS_H
#define FOC_TRANSFORMS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Preserve float fields and order from Legacy USER/ipark.h. */
typedef struct { float I_alpha, I_beta; } FOC_AlphaBetaCurrents;
typedef struct { float I_q, I_d; } FOC_DQCurrents;
typedef struct { float d, q; } FOC_DQVoltage;
typedef struct { float alpha, beta; } FOC_AlphaBetaVoltage;

/* Pure arithmetic from SYSTEM/ipark.c; inputs replace shared globals.
 * Park's q_current diagnostic mirror is not wired; I_q is returned explicitly.
 * sinAngle/cosAngle are supplied as in Legacy, with no angle convention change. */
void FOC_Clarke(float u_current, float v_current, FOC_AlphaBetaCurrents *ab_currents);
void FOC_Park(const FOC_AlphaBetaCurrents *ab_currents, float sinAngle,
              float cosAngle, FOC_DQCurrents *dq_currents);
void FOC_InversePark(const FOC_DQVoltage *dq, FOC_AlphaBetaVoltage *alphaBeta,
                     float sinAngle, float cosAngle);

#ifdef __cplusplus
}
#endif

#endif
